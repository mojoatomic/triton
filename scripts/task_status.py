#!/usr/bin/env python3
"""
Track task completion status for RC Submarine Controller project.

Scans project directory and checks which tasks are complete based on:
- File existence
- Minimum code content (not just stubs)
- Compilation success (optional)

Usage:
    python task_status.py <project-path>
    python task_status.py <project-path> --verbose
    
Example:
    python task_status.py ~/projects/rc-sub-controller
"""

import os
import sys
import re
import argparse
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class Task:
    id: str
    name: str
    description: str
    check_files: List[str]  # Files that should exist
    min_lines: int = 20     # Minimum non-comment lines to be "complete"
    
    
# Task definitions
TASKS = [
    # Foundation
    Task("F1", "Project scaffold", "CMakeLists.txt and directory structure",
         ["CMakeLists.txt", "src/config.h", "src/types.h"]),
    Task("F2", "Blink test", "LED blink confirms toolchain",
         ["src/main.c"], min_lines=15),
    Task("F3", "Dual-core hello", "Both cores running",
         ["src/main.c"], min_lines=30),
    Task("F4", "Config header", "Pin assignments and constants",
         ["src/config.h"], min_lines=50),
    Task("F5", "Types header", "Shared data types",
         ["src/types.h"], min_lines=50),
    
    # Drivers
    Task("D1", "RC input driver", "PIO PWM capture",
         ["src/drivers/rc_input.c", "pio/pwm_capture.pio"], min_lines=50),
    Task("D2", "Pressure sensor", "MS5837 I2C driver",
         ["src/drivers/pressure_sensor.c"], min_lines=80),
    Task("D3", "IMU driver", "MPU-6050 I2C driver",
         ["src/drivers/imu.c"], min_lines=80),
    Task("D4", "Pump driver", "PWM output control",
         ["src/drivers/pump.c"], min_lines=30),
    Task("D5", "Valve driver", "GPIO control",
         ["src/drivers/valve.c"], min_lines=20),
    Task("D6", "Servo driver", "PWM servo output",
         ["src/drivers/servo.c"], min_lines=40),
    Task("D7", "Battery monitor", "ADC voltage reading",
         ["src/drivers/battery.c"], min_lines=25),
    Task("D8", "Leak detector", "GPIO interrupt",
         ["src/drivers/leak.c"], min_lines=25),
    
    # Control
    Task("C1", "PID module", "Generic PID controller",
         ["src/control/pid.c"], min_lines=50),
    Task("C2", "Depth controller", "Depth hold using PID",
         ["src/control/depth_ctrl.c"], min_lines=40),
    Task("C3", "Pitch controller", "Pitch stabilization",
         ["src/control/pitch_ctrl.c"], min_lines=40),
    Task("C4", "Ballast state machine", "Fill/drain/hold states",
         ["src/control/ballast_ctrl.c"], min_lines=60),
    Task("C5", "Main state machine", "Overall submarine states",
         ["src/control/state_machine.c"], min_lines=80),
    
    # Safety
    Task("S1", "Safety monitor", "Core 0 fault checking",
         ["src/safety/safety_monitor.c"], min_lines=60),
    Task("S2", "Emergency blow", "Emergency surface sequence",
         ["src/safety/emergency.c"], min_lines=40),
    Task("S3", "Watchdog setup", "Hardware watchdog",
         ["src/safety/safety_monitor.c"], min_lines=70),  # Part of safety monitor
    Task("S4", "Fault logging", "Event log ring buffer",
         ["src/util/log.c"], min_lines=50),
]


def count_code_lines(filepath: str) -> int:
    """Count non-empty, non-comment lines in a C file."""
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
    except:
        return 0
    
    code_lines = 0
    in_block_comment = False
    
    for line in lines:
        line = line.strip()
        
        # Handle block comments
        if '/*' in line and '*/' not in line:
            in_block_comment = True
            continue
        if '*/' in line:
            in_block_comment = False
            continue
        if in_block_comment:
            continue
            
        # Skip empty lines and single-line comments
        if not line or line.startswith('//') or line.startswith('/*'):
            continue
            
        # Skip preprocessor directives (mostly)
        if line.startswith('#include') or line.startswith('#define'):
            continue
            
        code_lines += 1
    
    return code_lines


def check_task(task: Task, project_path: str, verbose: bool = False) -> bool:
    """Check if a task is complete."""
    
    # Check all required files exist
    for rel_path in task.check_files:
        full_path = os.path.join(project_path, rel_path)
        if not os.path.exists(full_path):
            if verbose:
                print(f"    Missing: {rel_path}")
            return False
    
    # Check minimum code content in primary file
    primary_file = task.check_files[0]
    full_path = os.path.join(project_path, primary_file)
    code_lines = count_code_lines(full_path)
    
    if code_lines < task.min_lines:
        if verbose:
            print(f"    {primary_file}: {code_lines} lines (need {task.min_lines})")
        return False
    
    return True


def print_status(tasks: List[Task], project_path: str, verbose: bool = False):
    """Print task completion status."""
    
    # Group tasks by category
    categories = {
        'F': ('Foundation', []),
        'D': ('Drivers', []),
        'C': ('Control', []),
        'S': ('Safety', []),
        'I': ('Integration', []),
    }
    
    for task in tasks:
        cat = task.id[0]
        if cat in categories:
            categories[cat][1].append(task)
    
    total = 0
    complete = 0
    
    for cat_id, (cat_name, cat_tasks) in categories.items():
        if not cat_tasks:
            continue
            
        print(f"\n{cat_name} Tasks:")
        print("-" * 50)
        
        for task in cat_tasks:
            total += 1
            is_complete = check_task(task, project_path, verbose)
            
            if is_complete:
                complete += 1
                status = "✓"
                color = "\033[92m"  # Green
            else:
                status = "○"
                color = "\033[91m"  # Red
            
            reset = "\033[0m"
            print(f"  {color}[{status}]{reset} {task.id}: {task.name}")
            
            if verbose and not is_complete:
                check_task(task, project_path, verbose=True)
    
    # Summary
    print(f"\n{'='*50}")
    pct = (complete / total * 100) if total > 0 else 0
    print(f"Progress: {complete}/{total} tasks complete ({pct:.0f}%)")
    
    # Progress bar
    bar_len = 40
    filled = int(bar_len * complete / total) if total > 0 else 0
    bar = "█" * filled + "░" * (bar_len - filled)
    print(f"[{bar}]")


def main():
    parser = argparse.ArgumentParser(description="RC Submarine task tracker")
    parser.add_argument("path", help="Project directory path")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show details for incomplete tasks")
    args = parser.parse_args()
    
    project_path = os.path.abspath(args.path)
    
    if not os.path.isdir(project_path):
        print(f"Error: Directory not found: {project_path}")
        sys.exit(1)
    
    print(f"RC Submarine Controller - Task Status")
    print(f"Project: {project_path}")
    
    print_status(TASKS, project_path, args.verbose)


if __name__ == "__main__":
    main()
