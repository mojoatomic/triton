---
name: github-hygiene
description: Enforces strict GitHub workflow hygiene and best practices. Use this skill for ALL git and GitHub operations including commits, branches, pull requests, and repository management. Core rules - NEVER commit directly to main/master, ALWAYS work on feature branches, use GitHub MCP for all GitHub API operations (issues, PRs, reviews), require PRs for all merges. Triggers on any git command, GitHub operation, or when starting new feature work.
license: MIT
---

# GitHub Hygiene

Enforce disciplined git workflow. No exceptions. No shortcuts.

## Critical Rules

### NEVER DO - Hard Stops

```bash
# These commands on main/master require EXPLICIT user override
git commit          # on main/master
git push origin main
git push origin master
git merge <branch>  # while on main/master
git push --force    # on ANY protected branch
git reset --hard    # without explicit confirmation
```

**Before ANY git operation:** Check current branch first.

```bash
branch=$(git branch --show-current)
if [[ "$branch" == "main" || "$branch" == "master" ]]; then
    # STOP - Require explicit user confirmation before proceeding
    echo "⛔ On protected branch: $branch"
fi
```

### ALWAYS DO - Mandatory Workflow

1. **Start work on a branch** - Create feature branch BEFORE any code changes
2. **Use GitHub MCP** - For all GitHub API operations (issues, PRs, reviews, checks)
3. **Atomic commits** - One logical change per commit
4. **PR for everything** - All changes reach main via pull request
5. **Delete merged branches** - Clean up after merge

## Branch Workflow

### Starting New Work

```bash
# 1. Ensure main is current
git checkout main
git pull origin main

# 2. Create feature branch (REQUIRED before any changes)
git checkout -b <branch-type>/<description>

# Branch naming convention:
# feature/   - New functionality
# fix/       - Bug fixes
# hotfix/    - Urgent production fixes
# refactor/  - Code restructuring
# docs/      - Documentation only
# test/      - Test additions/changes
# chore/     - Maintenance tasks
```

### Branch Naming Rules

Format: `<type>/<ticket-or-description>`

Good:
- `feature/user-authentication`
- `fix/pathmon-timeout-handling`
- `hotfix/triton-startup-crash`
- `refactor/secflo-rls-policies`

Bad:
- `my-changes` (no type prefix)
- `feature/stuff` (vague description)
- `Feature/Auth` (wrong case)

## Commit Standards

### Conventional Commits (Required)

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat` - New feature
- `fix` - Bug fix
- `docs` - Documentation only
- `style` - Formatting, no code change
- `refactor` - Code restructuring
- `test` - Adding/updating tests
- `chore` - Maintenance tasks
- `perf` - Performance improvement

**Examples:**
```
feat(triton): add dual-core handshake timeout handling

Implements NASA JPL Power of 10 compliant timeout mechanism
for Pico core synchronization during startup sequence.

Closes #42
```

```
fix(pathmon): correct SLA evidence timestamp format

FCC complaint export now uses ISO 8601 format consistently.
```

### Commit Hygiene

- **Atomic:** One logical change per commit
- **Present tense:** "add feature" not "added feature"
- **No period:** At end of subject line
- **72 chars:** Maximum subject line length
- **Body:** Explain what and why, not how

## GitHub MCP Usage

**ALWAYS use GitHub MCP for these operations:**

| Operation | MCP Tool | NOT This |
|-----------|----------|----------|
| Create issue | `create_issue` | GitHub web UI during session |
| Create PR | `create_pull_request` | `gh pr create` without MCP |
| List PRs | `list_pull_requests` | Manual checking |
| Get PR details | `get_pull_request` | `gh pr view` |
| PR reviews | `create_pull_request_review` | Web UI |
| Check CI status | `get_pull_request` | Manually refreshing |
| Merge PR | `merge_pull_request` | `gh pr merge` without MCP |

**Why MCP over CLI:**
- Maintains context in conversation
- Structured data for follow-up operations
- Audit trail in chat history
- Enables intelligent automation

### MCP Setup Verification

Before GitHub operations, verify MCP is available:

```
/mcp
```

If GitHub MCP not configured, alert user:
```
⚠️ GitHub MCP not configured. 
Add with: claude mcp add --transport http github https://api.githubcopilot.com/mcp -H "Authorization: Bearer YOUR_PAT"
```

## Pull Request Workflow

### Creating PRs

1. Push branch to origin
2. Use GitHub MCP `create_pull_request`
3. Include in PR body:
   - Summary of changes
   - Testing performed
   - Related issues (Closes #XX)
   - Breaking changes (if any)

### PR Checklist (Verify Before Creating)

- [ ] Branch is up to date with main
- [ ] All tests pass locally
- [ ] No merge conflicts
- [ ] Commit history is clean (squash if needed)
- [ ] Description explains the "why"

### After PR Merge

```bash
# Switch to main and update
git checkout main
git pull origin main

# Delete local feature branch
git branch -d <branch-name>

# Delete remote feature branch (if not auto-deleted)
git push origin --delete <branch-name>
```

## Safety Checks

### Pre-Commit Check

Run before every commit operation:

```bash
#!/bin/bash
# Reject commits on protected branches
branch=$(git branch --show-current)
protected=("main" "master" "production" "release")

for p in "${protected[@]}"; do
    if [[ "$branch" == "$p" ]]; then
        echo "❌ Direct commits to '$branch' are not allowed"
        echo "Create a feature branch: git checkout -b feature/your-change"
        exit 1
    fi
done
```

### Pre-Push Check

Verify before pushing:

1. Correct remote branch target
2. No force push to protected branches
3. Branch exists on remote (for updates)

## Recovery Procedures

### Accidentally Committed to Main

```bash
# If not yet pushed - move commit to new branch
git branch feature/rescue-commit
git reset --hard HEAD~1
git checkout feature/rescue-commit
```

### Accidentally Pushed to Main

```bash
# STOP - Do not force push
# 1. Create PR to revert
# 2. Or coordinate with team for proper resolution
# Force push to main is NEVER acceptable without explicit team approval
```

## Quick Reference

| Situation | Action |
|-----------|--------|
| Starting new feature | `git checkout -b feature/<name>` |
| Ready to commit | Check branch first, then commit |
| Ready to merge | Create PR via GitHub MCP |
| PR approved | Merge via MCP, delete branch |
| On wrong branch | Stash, switch, apply |
| Need main updates | `git pull origin main` (from feature branch: `git rebase main`) |

## Red Flags - Stop and Verify

- About to run `git push origin main`
- About to run `git commit` while on main/master
- About to run `git push --force` on any branch
- About to run `git reset --hard` anywhere
- GitHub MCP not responding - do not fall back to CLI without noting

## Integration Notes

**Pairs with:**
- `using-git-worktrees` - For isolated development environments
- `test-driven-development` - Tests pass before commits
- `finishing-a-development-branch` - Proper branch cleanup

**Announce at start:** "Using github-hygiene skill to ensure proper git workflow."
