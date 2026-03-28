#!/usr/bin/env python3
"""
Script to replace make_optional() calls with direct optional<T>() construction.
This fixes compilation errors with C++17's std::decay_t.
"""

import re
import sys
from pathlib import Path

def fix_file(filepath):
    """Fix make_optional calls in a single file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original = content
        
        # Replace make_optional(arg) with appropriate syntax
        # This is a simple pattern - won't catch all cases but will fix most
        content = re.sub(
            r'\bmake_optional\b',
            'optional',
            content
        )
        
        # Also replace std::make_optional
        content = re.sub(
            r'std::make_optional\b',
            'optional',
            content
        )
        
        if content != original:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Fixed: {filepath}")
            return True
        return False
    except Exception as e:
        print(f"Error processing {filepath}: {e}", file=sys.stderr)
        return False

def main():
    """Main function to process all .cc and .tcc files."""
    src_dir = Path('/Users/anton.feldmann/Projects/priv/rethinkdb/src')
    
    fixed_count = 0
    for pattern in ['**/*.cc', '**/*.tcc', '**/*.hpp']:
        for filepath in src_dir.glob(pattern):
            if fix_file(filepath):
                fixed_count += 1
    
    print(f"\nTotal files fixed: {fixed_count}")

if __name__ == '__main__':
    main()
