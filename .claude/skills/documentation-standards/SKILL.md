---
name: documentation-standards
description: Apply professional documentation standards. Use when writing README files, commit messages, code comments, technical docs, or any user-facing text. Enforces evidence-based claims, no marketing language.
---

# Documentation Standards

Technical documentation must be factually accurate, conservative in claims, and free of marketing language.

## Formatting Rules

No emojis. No checkmarks, party poppers, or unicode decorations. Use plain text: "Compliant", "Implemented", "Supported", "Pending".

No excessive formatting. Use bold sparingly for genuine emphasis only.

## Prohibited Language

### Marketing Hyperbole - Never Use

- "Production-ready"
- "Enterprise-grade"
- "Best-in-class"
- "Industry-leading"
- "State-of-the-art"
- "Cutting-edge"
- "Revolutionary"
- "World-class"
- "Unparalleled"
- "Seamless"
- "Robust" (without specific meaning)
- "Powerful" (without quantification)

### Absolute Claims - Never Use

- "Perfect"
- "Flawless"
- "100% accurate"
- "Guaranteed"
- "Always works"
- "Never fails"
- "Completely secure"

### Competitive Claims - Never Use

- "Better than X"
- "Superior to alternatives"
- "More accurate than other tools"
- "Faster than existing solutions"

### Vague Endorsement - Never Use

- "NASA-approved" (unless actually approved)
- "Certified accurate"
- "Validated by experts"
- "Industry-standard"

## Required Patterns

### Evidence-Backed Statements

| Bad | Good |
|-----|------|
| "Highly accurate" | "Median error: 1.4 arcseconds across 7 bodies" |
| "Fast performance" | "Typical response time: 25-75ms" |
| "Well-tested" | "33 unit tests, 6 test suites, 100% pass rate" |
| "Production-ready" | "Deployed in [context]; further testing recommended for [other contexts]" |

### Bounded Statements

| Bad | Good |
|-----|------|
| "Low latency" | "< 100ms typical, < 500ms worst case" |
| "High precision" | "1.4 arcsec median, 8.6 arcsec maximum" |
| "Comprehensive coverage" | "87% line coverage, 92% branch coverage" |

### Honest Limitations

Always include a limitations section stating what the software does NOT do.

### Specific Methodology

When describing validation: state comparison target, methodology, dataset, and link to reproducible tests.

## Commit Messages

No emojis. Use conventional commits format:

```
feat: add depth controller PID implementation
fix: resolve sleep_us variable shadowing
docs: update validation methodology section
test: add unit tests for state machine transitions
```

Do not editorialize. No "awesome", no exclamation points.

## Code Comments

Comments explain WHY, not WHAT. No cheerleading.

```c
// Bad
// This awesome function does the magic!

// Good  
// Apply temperature compensation per MS5837 datasheet section 4.2.
```

No TODO without ownership:

```c
// Bad
// TODO: fix this later

// Good
// TODO(doug): Add timeout handling - see issue #47
```

## README Structure

```markdown
# Project Name

One-sentence factual description.

**Status:** Development / Testing / Stable
**Validation:** [Specific methodology and results]

## What This Does

[Factual description]

## Limitations

[Explicit statement of what it doesn't do]

## Validation

[Methodology, data sources, reproducible tests]

## Known Issues

[Current problems]
```

## Pull Request Descriptions

State what changed and why. No self-congratulation.

```markdown
## Add Core 1 health monitoring

Adds heartbeat-based detection of Core 1 stalls.

Changes:
- Add g_core1_heartbeat volatile counter
- Add check_core1_health() to safety monitor
- Add EVT_CORE1_STALL event code

Testing:
- Unit test: mock heartbeat stall, verify emergency trigger
```

## Summary

Write as if reviewed by a skeptical engineer who will challenge every unsubstantiated claim.
