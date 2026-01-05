#!/usr/bin/env python3
"""
Validate C code for Power of 10 compliance.

Checks:
1. No dynamic memory (malloc, free, realloc, calloc)
2. Loop bounds (all loops should have fixed iteration limits)
3. Function length (max 60 lines recommended)
4. Assert presence (at least 1 per function)
5. Return value checking patterns

Usage:
    python validate_code.py <file.c>
    python validate_code.py <directory>
    
Example:
    python validate_code.py src/drivers/pump.c
    python validate_code.py src/
"""

import os
import sys
import re
import argparse
from dataclasses import dataclass
from typing import List, Tuple


@dataclass
class Violation:
    file: str
    line: int
    rule: str
    message: str
    severity: str  # "error" or "warning"


class P10Validator:
    """Power of 10 code validator."""
    
    # Forbidden functions (Rule 1: No dynamic memory)
    FORBIDDEN_FUNCS = [
        'malloc', 'free', 'realloc', 'calloc',
        'new', 'delete',  # C++
    ]
    
    # Patterns that suggest unbounded loops
    UNBOUNDED_LOOP_PATTERNS = [
        r'while\s*\(\s*1\s*\)',
        r'while\s*\(\s*true\s*\)',
        r'for\s*\(\s*;\s*;\s*\)',
    ]
    
    def __init__(self):
        self.violations: List[Violation] = []
        
    def validate_file(self, filepath: str) -> List[Violation]:
        """Validate a single C file."""
        self.violations = []
        
        try:
            with open(filepath, 'r') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            self.violations.append(Violation(
                file=filepath, line=0, rule="FILE",
                message=f"Could not read file: {e}",
                severity="error"
            ))
            return self.violations
        
        self._check_forbidden_functions(filepath, content, lines)
        self._check_unbounded_loops(filepath, content, lines)
        self._check_function_length(filepath, content, lines)
        self._check_asserts(filepath, content, lines)
        self._check_return_values(filepath, content, lines)
        
        return self.violations
    
    def _check_forbidden_functions(self, filepath: str, content: str, lines: List[str]):
        """Rule 1: No dynamic memory allocation."""
        for func in self.FORBIDDEN_FUNCS:
            # Match function calls (not in comments)
            pattern = rf'\b{func}\s*\('
            for i, line in enumerate(lines, 1):
                # Skip comments
                code = self._strip_comments(line)
                if re.search(pattern, code):
                    self.violations.append(Violation(
                        file=filepath, line=i, rule="P10-1",
                        message=f"Forbidden function '{func}' (no dynamic memory)",
                        severity="error"
                    ))
    
    def _check_unbounded_loops(self, filepath: str, content: str, lines: List[str]):
        """Rule 2: All loops must have fixed bounds."""
        for pattern in self.UNBOUNDED_LOOP_PATTERNS:
            for i, line in enumerate(lines, 1):
                code = self._strip_comments(line)
                if re.search(pattern, code):
                    self.violations.append(Violation(
                        file=filepath, line=i, rule="P10-2",
                        message="Potentially unbounded loop detected",
                        severity="warning"
                    ))
        
        # Check for while loops without obvious bounds
        for i, line in enumerate(lines, 1):
            code = self._strip_comments(line)
            match = re.search(r'while\s*\(([^)]+)\)', code)
            if match:
                condition = match.group(1).strip()
                # Check if it looks like a timeout or counter pattern
                has_bound = any(p in condition for p in [
                    '<', '>', '<=', '>=', '!=', '--', '++',
                    'timeout', 'count', 'retry', 'attempt', 'iter'
                ])
                if not has_bound and condition not in ['0', 'false']:
                    self.violations.append(Violation(
                        file=filepath, line=i, rule="P10-2",
                        message=f"Loop may lack explicit bound: while({condition})",
                        severity="warning"
                    ))
    
    def _check_function_length(self, filepath: str, content: str, lines: List[str]):
        """Rule 3: Functions should be ≤60 lines."""
        in_function = False
        func_start = 0
        func_name = ""
        brace_depth = 0
        
        for i, line in enumerate(lines, 1):
            code = self._strip_comments(line)
            
            # Detect function definition
            if not in_function:
                # Simple heuristic: type + name + ( at start of line
                match = re.match(r'^[a-zA-Z_][a-zA-Z0-9_*\s]+\s+(\w+)\s*\([^;]*$', code)
                if match and '{' in code:
                    in_function = True
                    func_start = i
                    func_name = match.group(1)
                    brace_depth = code.count('{') - code.count('}')
                elif match:
                    # Function definition continues on next line
                    func_name = match.group(1)
            else:
                brace_depth += code.count('{') - code.count('}')
                
                if brace_depth <= 0:
                    # End of function
                    func_length = i - func_start + 1
                    if func_length > 60:
                        self.violations.append(Violation(
                            file=filepath, line=func_start, rule="P10-3",
                            message=f"Function '{func_name}' is {func_length} lines (max 60)",
                            severity="warning"
                        ))
                    in_function = False
                    brace_depth = 0
    
    def _check_asserts(self, filepath: str, content: str, lines: List[str]):
        """Rule 6: At least 1 assert per function."""
        functions = self._find_functions(content)
        
        for func_name, start, end in functions:
            func_content = '\n'.join(lines[start-1:end])
            
            # Check for P10_ASSERT or assert
            has_assert = bool(re.search(r'\b(P10_ASSERT|assert)\s*\(', func_content))
            
            if not has_assert:
                self.violations.append(Violation(
                    file=filepath, line=start, rule="P10-6",
                    message=f"Function '{func_name}' has no assertions",
                    severity="warning"
                ))
    
    def _check_return_values(self, filepath: str, content: str, lines: List[str]):
        """Rule 4: Check return values (heuristic)."""
        # Look for common patterns where return values might be ignored
        ignored_patterns = [
            # Function call at start of line (not assigned)
            r'^\s+(?!return|if|while|for|switch)(i2c_|spi_|gpio_|adc_|pwm_)\w+\s*\([^;]+\);',
        ]
        
        for i, line in enumerate(lines, 1):
            code = self._strip_comments(line)
            for pattern in ignored_patterns:
                if re.search(pattern, code):
                    # Extract function name
                    match = re.search(r'(i2c_|spi_|gpio_|adc_|pwm_)\w+', code)
                    if match:
                        self.violations.append(Violation(
                            file=filepath, line=i, rule="P10-4",
                            message=f"Return value of '{match.group(0)}' may be ignored",
                            severity="warning"
                        ))
    
    def _find_functions(self, content: str) -> List[Tuple[str, int, int]]:
        """Find all function definitions and their line ranges."""
        functions = []
        lines = content.split('\n')
        
        in_function = False
        func_start = 0
        func_name = ""
        brace_depth = 0
        
        for i, line in enumerate(lines, 1):
            code = self._strip_comments(line)
            
            if not in_function:
                match = re.match(r'^[a-zA-Z_][a-zA-Z0-9_*\s]+\s+(\w+)\s*\([^;]*\{', code)
                if match:
                    in_function = True
                    func_start = i
                    func_name = match.group(1)
                    brace_depth = code.count('{') - code.count('}')
            else:
                brace_depth += code.count('{') - code.count('}')
                if brace_depth <= 0:
                    functions.append((func_name, func_start, i))
                    in_function = False
        
        return functions
    
    def _strip_comments(self, line: str) -> str:
        """Remove C-style comments from a line."""
        # Remove // comments
        line = re.sub(r'//.*$', '', line)
        # Remove /* */ comments (single line only)
        line = re.sub(r'/\*.*?\*/', '', line)
        return line


def main():
    parser = argparse.ArgumentParser(description="Power of 10 C code validator")
    parser.add_argument("path", help="File or directory to validate")
    parser.add_argument("--strict", action="store_true", help="Treat warnings as errors")
    args = parser.parse_args()
    
    validator = P10Validator()
    all_violations = []
    
    if os.path.isfile(args.path):
        files = [args.path]
    elif os.path.isdir(args.path):
        files = []
        for root, _, filenames in os.walk(args.path):
            for filename in filenames:
                if filename.endswith('.c') or filename.endswith('.h'):
                    files.append(os.path.join(root, filename))
    else:
        print(f"Error: Path not found: {args.path}")
        sys.exit(1)
    
    for filepath in files:
        violations = validator.validate_file(filepath)
        all_violations.extend(violations)
    
    # Print results
    errors = [v for v in all_violations if v.severity == "error"]
    warnings = [v for v in all_violations if v.severity == "warning"]
    
    for v in all_violations:
        prefix = "ERROR" if v.severity == "error" else "WARN"
        print(f"{v.file}:{v.line}: [{prefix}] {v.rule}: {v.message}")
    
    print(f"\n{'='*60}")
    print(f"Files checked: {len(files)}")
    print(f"Errors: {len(errors)}")
    print(f"Warnings: {len(warnings)}")
    
    if errors or (args.strict and warnings):
        print("\nValidation FAILED")
        sys.exit(1)
    else:
        print("\nValidation PASSED")
        sys.exit(0)


if __name__ == "__main__":
    main()
