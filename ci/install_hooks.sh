#!/bin/bash
#
# Install git hooks for RC Submarine Controller
#
# Usage:
#   ./ci/install_hooks.sh           # Install hooks
#   ./ci/install_hooks.sh --remove  # Remove hooks
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
GIT_DIR="$PROJECT_ROOT/.git"
HOOKS_DIR="$GIT_DIR/hooks"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [[ ! -d "$GIT_DIR" ]]; then
    echo -e "${RED}Error: Not a git repository${NC}"
    echo "Run 'git init' first"
    exit 1
fi

if [[ "$1" == "--remove" ]]; then
    echo "Removing git hooks..."
    
    for hook in pre-commit pre-push; do
        if [[ -L "$HOOKS_DIR/$hook" ]]; then
            rm "$HOOKS_DIR/$hook"
            echo -e "${GREEN}  ✓ Removed $hook${NC}"
        elif [[ -f "$HOOKS_DIR/$hook" ]]; then
            # Check if it's our hook
            if grep -q "RC Submarine Controller" "$HOOKS_DIR/$hook" 2>/dev/null; then
                rm "$HOOKS_DIR/$hook"
                echo -e "${GREEN}  ✓ Removed $hook${NC}"
            else
                echo -e "${YELLOW}  ○ $hook exists but is not our hook, skipping${NC}"
            fi
        fi
    done
    
    echo -e "${GREEN}Hooks removed${NC}"
    exit 0
fi

echo "Installing git hooks..."

# Create hooks directory if needed
mkdir -p "$HOOKS_DIR"

# Install pre-commit hook
SOURCE_HOOK="$SCRIPT_DIR/hooks/pre-commit"
TARGET_HOOK="$HOOKS_DIR/pre-commit"

if [[ -f "$SOURCE_HOOK" ]]; then
    if [[ -f "$TARGET_HOOK" ]]; then
        # Check if it's a symlink to our hook or our hook content
        if [[ -L "$TARGET_HOOK" ]]; then
            rm "$TARGET_HOOK"
        elif grep -q "RC Submarine Controller" "$TARGET_HOOK" 2>/dev/null; then
            rm "$TARGET_HOOK"
        else
            echo -e "${YELLOW}Warning: Existing pre-commit hook found${NC}"
            echo "  Backing up to pre-commit.backup"
            mv "$TARGET_HOOK" "$TARGET_HOOK.backup"
        fi
    fi
    
    # Create symlink to source hook
    ln -s "$SOURCE_HOOK" "$TARGET_HOOK"
    chmod +x "$SOURCE_HOOK"
    echo -e "${GREEN}  ✓ Installed pre-commit hook${NC}"
else
    echo -e "${RED}  ✗ Source hook not found: $SOURCE_HOOK${NC}"
fi

# Create pre-push hook (runs full CI)
cat > "$HOOKS_DIR/pre-push" << 'EOF'
#!/bin/bash
#
# Git pre-push hook for RC Submarine Controller
# Runs full CI before pushing
#

PROJECT_ROOT="$(git rev-parse --show-toplevel)"
CI_SCRIPT="$PROJECT_ROOT/ci/run_ci.sh"

if [[ -f "$CI_SCRIPT" ]]; then
    echo "Running full CI before push..."
    bash "$CI_SCRIPT" --quick
else
    echo "Warning: CI script not found, skipping pre-push checks"
fi
EOF

chmod +x "$HOOKS_DIR/pre-push"
echo -e "${GREEN}  ✓ Installed pre-push hook${NC}"

echo ""
echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  Git hooks installed successfully!${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
echo ""
echo "Hooks will run automatically on:"
echo "  - git commit  → Quick format and P10 checks"
echo "  - git push    → Full CI (--quick mode)"
echo ""
echo "To bypass hooks (not recommended):"
echo "  git commit --no-verify"
echo "  git push --no-verify"
echo ""
echo "To remove hooks:"
echo "  ./ci/install_hooks.sh --remove"
