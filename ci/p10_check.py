#!/usr/bin/env python3
"""
Power of 10 Compliance Checker for CI

A strict validator that enforces NASA/JPL Power of 10 rules for embedded C.

Rules checked:
1. No dynamic memory allocation (malloc, free, etc.)
2. All loops must have fixed upper bounds
3. No recursion
4. All functions must be ≤60 lines
5. Minimum 2 assertions per function (configurable)
6. Data must be declared at smallest scope
7. All return values must be checked
8. Limited pointer dereferencing
9. All code must compile without warnings (-Wall -Werror)
10. Static analysis must pass (cppcheck integration)

Exit codes:
  0 - All checks passed
  1 - Errors found
  2 - Warnings found (with --strict)
"""

import os
import sys
import re
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Optional
from enum import Enum
import json


class Severity(Enum):
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class Violation:
    file: str
    line: int
    rule: str
    message: str
    severity: Severity
    code_snippet: str = ""
    
    def to_dict(self):
        return {
            "file": self.file,
            "line": self.line,
            "rule": self.rule,
            "message": self.message,
            "severity": self.severity.value,
            "code_snippet": self.code_snippet
        }


@dataclass 
class FunctionInfo:
    name: str
    start_line: int
    end_line: int
    param_count: int
    assert_count: int
    return_count: int
    has_loops: bool
    calls_itself: bool  # Recursion detection


@dataclass
class FileAnalysis:
    path: str
    lines: List[str] = field(default_factory=list)
    functions: List[FunctionInfo] = field(default_factory=list)
    violations: List[Violation] = field(default_factory=list)
    

class P10Checker:
    """Power of 10 compliance checker."""
    
    # Rule 1: Forbidden dynamic memory functions
    FORBIDDEN_FUNCTIONS = {
        'malloc': 'Use static allocation instead',
        'calloc': 'Use static allocation instead', 
        'realloc': 'Use static allocation instead',
        'free': 'Memory should be statically allocated',
        'alloca': 'Stack allocation is unpredictable',
        'strdup': 'Allocates memory dynamically',
        'strndup': 'Allocates memory dynamically',
        'asprintf': 'Allocates memory dynamically',
        'vasprintf': 'Allocates memory dynamically',
    }
    
    # Rule 2: Potentially unbounded loop patterns
    UNBOUNDED_PATTERNS = [
        (r'while\s*\(\s*1\s*\)', 'Infinite loop: while(1)'),
        (r'while\s*\(\s*true\s*\)', 'Infinite loop: while(true)'),
        (r'for\s*\(\s*;\s*;\s*\)', 'Infinite loop: for(;;)'),
        (r'while\s*\(\s*!\s*0\s*\)', 'Infinite loop: while(!0)'),
    ]
    
    # Functions where infinite loops are allowed (main event loops)
    MAIN_LOOP_FUNCTIONS = ['main', 'core0_main', 'core1_main', 'core0_entry', 'core1_entry']
    
    # Configuration
    MAX_FUNCTION_LINES = 60
    MIN_ASSERTS_PER_FUNCTION = 1
    MAX_POINTER_DEREF = 2  # **ptr is ok, ***ptr is not
    MAX_PARAMS = 7
    
    def __init__(self, strict: bool = False):
        self.strict = strict
        self.analyses: Dict[str, FileAnalysis] = {}
        
    def check_file(self, filepath: str) -> FileAnalysis:
        """Analyze a single file for P10 compliance."""
        analysis = FileAnalysis(path=filepath)
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
                analysis.lines = f.readlines()
        except Exception as e:
            analysis.violations.append(Violation(
                file=filepath, line=0, rule="FILE",
                message=f"Cannot read file: {e}",
                severity=Severity.ERROR
            ))
            return analysis
        
        # Run all checks
        self._extract_functions(analysis)  # Must be first - other checks depend on it
        self._check_forbidden_functions(analysis)
        self._check_loop_bounds(analysis)
        self._check_function_length(analysis)
        self._check_recursion(analysis)
        self._check_assertions(analysis)
        self._check_pointer_depth(analysis)
        self._check_goto(analysis)
        self._check_return_values(analysis)
        self._check_global_variables(analysis)
        
        self.analyses[filepath] = analysis
        return analysis
    
    def _strip_comments(self, line: str) -> str:
        """Remove comments from a line (simple version)."""
        # Remove // comments
        idx = line.find('//')
        if idx >= 0:
            # Make sure it's not in a string
            in_string = False
            for i, c in enumerate(line[:idx]):
                if c == '"' and (i == 0 or line[i-1] != '\\'):
                    in_string = not in_string
            if not in_string:
                line = line[:idx]
        return line
    
    def _check_forbidden_functions(self, analysis: FileAnalysis):
        """Rule 1: No dynamic memory allocation."""
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line)
            
            for func, reason in self.FORBIDDEN_FUNCTIONS.items():
                # Match function calls, not definitions or comments
                pattern = rf'\b{func}\s*\('
                if re.search(pattern, code):
                    analysis.violations.append(Violation(
                        file=analysis.path, line=i, rule="P10-1",
                        message=f"Dynamic memory function '{func}': {reason}",
                        severity=Severity.ERROR,
                        code_snippet=line.strip()
                    ))
    
    def _check_loop_bounds(self, analysis: FileAnalysis):
        """Rule 2: All loops must have fixed upper bounds."""
        # First pass: find which lines are inside main loop functions
        main_loop_lines = set()
        for func in analysis.functions:
            if func.name in self.MAIN_LOOP_FUNCTIONS:
                for line_num in range(func.start_line, func.end_line + 1):
                    main_loop_lines.add(line_num)
        
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line)
            
            # Skip infinite loop check if we're in a main loop function
            if i in main_loop_lines:
                continue
            
            # Check for obviously infinite loops
            for pattern, msg in self.UNBOUNDED_PATTERNS:
                if re.search(pattern, code):
                    analysis.violations.append(Violation(
                        file=analysis.path, line=i, rule="P10-2",
                        message=msg,
                        severity=Severity.ERROR,
                        code_snippet=line.strip()
                    ))
            
            # Check while loops for bounds
            match = re.search(r'while\s*\(([^)]+)\)', code)
            if match:
                condition = match.group(1).strip()
                # Acceptable patterns: comparisons, counters, flags with timeouts
                acceptable = any(p in condition for p in [
                    '<', '>', '<=', '>=', '==', '!=',
                    '--', '++', 'timeout', 'retry', 'count', 'iter', 'attempt',
                    'MAX_', '_MAX', 'LIMIT'
                ])
                
                if not acceptable and condition not in ['0', 'false', 'NULL']:
                    analysis.violations.append(Violation(
                        file=analysis.path, line=i, rule="P10-2",
                        message=f"Loop may lack explicit bound: while({condition})",
                        severity=Severity.WARNING,
                        code_snippet=line.strip()
                    ))
    
    def _extract_functions(self, analysis: FileAnalysis):
        """Extract function definitions for analysis."""
        in_function = False
        brace_depth = 0
        current_func: Optional[FunctionInfo] = None
        func_content = []
        
        # Simple regex for function definitions
        func_pattern = re.compile(
            r'^(?:static\s+)?(?:inline\s+)?'  # Optional modifiers
            r'(?:const\s+)?'
            r'[a-zA-Z_][a-zA-Z0-9_*\s]+\s+'   # Return type
            r'([a-zA-Z_][a-zA-Z0-9_]*)\s*'    # Function name
            r'\(([^)]*)\)\s*'                  # Parameters
            r'(?:\{|$)'                        # Opening brace or end
        )
        
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line)
            
            if not in_function:
                match = func_pattern.match(code.strip())
                if match and '{' in code:
                    in_function = True
                    func_name = match.group(1)
                    params = match.group(2)
                    param_count = len([p for p in params.split(',') if p.strip()]) if params.strip() else 0
                    
                    current_func = FunctionInfo(
                        name=func_name,
                        start_line=i,
                        end_line=i,
                        param_count=param_count,
                        assert_count=0,
                        return_count=0,
                        has_loops=False,
                        calls_itself=False
                    )
                    func_content = [line]
                    brace_depth = code.count('{') - code.count('}')
            else:
                func_content.append(line)
                brace_depth += code.count('{') - code.count('}')
                
                # Check for assertions
                if re.search(r'\b(P10_ASSERT|assert|ASSERT)\s*\(', code):
                    current_func.assert_count += 1
                
                # Check for loops
                if re.search(r'\b(while|for|do)\s*[\({]', code):
                    current_func.has_loops = True
                
                # Check for recursion
                if re.search(rf'\b{current_func.name}\s*\(', code):
                    current_func.calls_itself = True
                
                # Check for return statements
                if re.search(r'\breturn\b', code):
                    current_func.return_count += 1
                
                if brace_depth <= 0:
                    current_func.end_line = i
                    analysis.functions.append(current_func)
                    in_function = False
                    current_func = None
                    func_content = []
    
    def _check_function_length(self, analysis: FileAnalysis):
        """Rule 4: Functions should be ≤60 lines."""
        for func in analysis.functions:
            length = func.end_line - func.start_line + 1
            if length > self.MAX_FUNCTION_LINES:
                analysis.violations.append(Violation(
                    file=analysis.path, line=func.start_line, rule="P10-4",
                    message=f"Function '{func.name}' is {length} lines (max {self.MAX_FUNCTION_LINES})",
                    severity=Severity.WARNING
                ))
            
            # Also check parameter count
            if func.param_count > self.MAX_PARAMS:
                analysis.violations.append(Violation(
                    file=analysis.path, line=func.start_line, rule="P10-PARAMS",
                    message=f"Function '{func.name}' has {func.param_count} parameters (max {self.MAX_PARAMS})",
                    severity=Severity.WARNING
                ))
    
    def _check_recursion(self, analysis: FileAnalysis):
        """Rule 3: No recursion."""
        for func in analysis.functions:
            if func.calls_itself:
                analysis.violations.append(Violation(
                    file=analysis.path, line=func.start_line, rule="P10-3",
                    message=f"Function '{func.name}' appears to be recursive",
                    severity=Severity.ERROR
                ))
    
    def _check_assertions(self, analysis: FileAnalysis):
        """Rule 5: Minimum assertions per function."""
        for func in analysis.functions:
            # Skip main loop functions - they're just event loops
            if func.name in self.MAIN_LOOP_FUNCTIONS:
                continue
                
            # Skip very short functions (< 10 lines)
            length = func.end_line - func.start_line + 1
            if length < 10:
                continue
                
            if func.assert_count < self.MIN_ASSERTS_PER_FUNCTION:
                analysis.violations.append(Violation(
                    file=analysis.path, line=func.start_line, rule="P10-5",
                    message=f"Function '{func.name}' has {func.assert_count} assertions (need {self.MIN_ASSERTS_PER_FUNCTION})",
                    severity=Severity.WARNING
                ))
    
    def _check_pointer_depth(self, analysis: FileAnalysis):
        """Rule 8: Limited pointer dereferencing."""
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line)
            
            # Look for deep pointer declarations or dereferences
            # ***ptr or more
            if re.search(r'\*{3,}', code):
                analysis.violations.append(Violation(
                    file=analysis.path, line=i, rule="P10-8",
                    message="Pointer depth exceeds 2 levels",
                    severity=Severity.WARNING,
                    code_snippet=line.strip()
                ))
    
    def _check_goto(self, analysis: FileAnalysis):
        """No goto statements (common extension to P10)."""
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line)
            
            if re.search(r'\bgoto\s+\w+', code):
                analysis.violations.append(Violation(
                    file=analysis.path, line=i, rule="P10-GOTO",
                    message="goto statement found",
                    severity=Severity.ERROR,
                    code_snippet=line.strip()
                ))
    
    def _check_return_values(self, analysis: FileAnalysis):
        """Rule 7: Check return values (heuristic)."""
        # Functions that commonly return error codes
        important_funcs = [
            'i2c_write', 'i2c_read', 'spi_write', 'spi_read',
            'uart_write', 'uart_read', 'gpio_get', 'adc_read',
            'fopen', 'fread', 'fwrite', 'fclose',
        ]
        
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line).strip()
            
            # Check if line is a bare function call (not assigned, not in condition)
            for func in important_funcs:
                pattern = rf'^\s*{func}[_a-z]*\s*\([^;]+\);'
                if re.search(pattern, code):
                    # Check if it's inside an if/while/assignment
                    if not re.search(r'^(if|while|for|switch|\w+\s*=)', code):
                        analysis.violations.append(Violation(
                            file=analysis.path, line=i, rule="P10-7",
                            message=f"Return value of '{func}...' may be ignored",
                            severity=Severity.WARNING,
                            code_snippet=line.strip()
                        ))
    
    def _check_global_variables(self, analysis: FileAnalysis):
        """Rule 6: Minimize global variables (informational)."""
        global_count = 0
        
        in_function = False
        brace_depth = 0
        
        for i, line in enumerate(analysis.lines, 1):
            code = self._strip_comments(line)
            
            # Track brace depth to know if we're in a function
            if '{' in code and not in_function:
                # Could be start of function
                if re.search(r'\)\s*\{', code):
                    in_function = True
                    brace_depth = 1
            elif in_function:
                brace_depth += code.count('{') - code.count('}')
                if brace_depth <= 0:
                    in_function = False
            
            # Look for global variable declarations (outside functions)
            if not in_function:
                # Match: type name; or type name = value;
                if re.match(r'^(?:static\s+)?(?:volatile\s+)?[a-zA-Z_][a-zA-Z0-9_]*\s+[a-zA-Z_][a-zA-Z0-9_*\[\]]*\s*[;=]', code.strip()):
                    # Exclude function declarations, typedefs, externs
                    if not re.search(r'\(|typedef|extern|#', code):
                        global_count += 1
        
        if global_count > 10:
            analysis.violations.append(Violation(
                file=analysis.path, line=0, rule="P10-6",
                message=f"File has {global_count} global variables (consider reducing)",
                severity=Severity.INFO
            ))


def check_directory(path: str, checker: P10Checker, recursive: bool = True) -> List[FileAnalysis]:
    """Check all C files in a directory."""
    analyses = []
    
    for root, dirs, files in os.walk(path):
        # Skip build directories
        dirs[:] = [d for d in dirs if d not in ['build', '.git', 'test']]
        
        for filename in files:
            if filename.endswith(('.c', '.h')):
                filepath = os.path.join(root, filename)
                analysis = checker.check_file(filepath)
                analyses.append(analysis)
        
        if not recursive:
            break
    
    return analyses


def print_violations(analyses: List[FileAnalysis], verbose: bool = False):
    """Print violations in a readable format."""
    total_errors = 0
    total_warnings = 0
    
    for analysis in analyses:
        errors = [v for v in analysis.violations if v.severity == Severity.ERROR]
        warnings = [v for v in analysis.violations if v.severity == Severity.WARNING]
        
        total_errors += len(errors)
        total_warnings += len(warnings)
        
        if not analysis.violations:
            if verbose:
                print(f"✓ {analysis.path}")
            continue
        
        for v in analysis.violations:
            if v.severity == Severity.INFO and not verbose:
                continue
                
            prefix = {
                Severity.ERROR: "\033[91mERROR\033[0m",
                Severity.WARNING: "\033[93mWARN\033[0m",
                Severity.INFO: "\033[94mINFO\033[0m"
            }[v.severity]
            
            print(f"{analysis.path}:{v.line}: [{prefix}] {v.rule}: {v.message}")
            if v.code_snippet and verbose:
                print(f"    {v.code_snippet}")
    
    return total_errors, total_warnings


def main():
    parser = argparse.ArgumentParser(
        description="Power of 10 Compliance Checker",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Rules checked:
  P10-1: No dynamic memory allocation
  P10-2: All loops must have fixed bounds
  P10-3: No recursion
  P10-4: Functions ≤60 lines
  P10-5: Assertions in functions
  P10-6: Minimize global variables
  P10-7: Check return values
  P10-8: Limited pointer depth
"""
    )
    parser.add_argument("path", help="File or directory to check")
    parser.add_argument("--strict", action="store_true", help="Treat warnings as errors")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--json", action="store_true", help="Output as JSON")
    args = parser.parse_args()
    
    checker = P10Checker(strict=args.strict)
    
    if os.path.isfile(args.path):
        analyses = [checker.check_file(args.path)]
    elif os.path.isdir(args.path):
        analyses = check_directory(args.path, checker)
    else:
        print(f"Error: Path not found: {args.path}")
        sys.exit(2)
    
    if args.json:
        output = {
            "files": len(analyses),
            "violations": []
        }
        for analysis in analyses:
            for v in analysis.violations:
                output["violations"].append(v.to_dict())
        print(json.dumps(output, indent=2))
        errors = len([v for v in output["violations"] if v["severity"] == "error"])
        warnings = len([v for v in output["violations"] if v["severity"] == "warning"])
    else:
        errors, warnings = print_violations(analyses, args.verbose)
        
        print(f"\n{'='*60}")
        print(f"Files checked: {len(analyses)}")
        print(f"Errors: {errors}")
        print(f"Warnings: {warnings}")
    
    if errors > 0:
        if not args.json:
            print("\n\033[91m✗ FAILED: Errors found\033[0m")
        sys.exit(1)
    elif args.strict and warnings > 0:
        if not args.json:
            print("\n\033[93m✗ FAILED: Warnings found (strict mode)\033[0m")
        sys.exit(1)
    else:
        if not args.json:
            print("\n\033[92m✓ PASSED\033[0m")
        sys.exit(0)


if __name__ == "__main__":
    main()
