# Contributing to chirp

Thank you for your interest in contributing to chirp!

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone git@github.com:YOUR_USERNAME/chirp.git`
3. Install pre-commit hooks: `pip install pre-commit && pre-commit install`
4. Create a feature branch: `git checkout -b feat/your-feature`

## Development Environment

### Required Tools

For firmware development:
- TI ARM Compiler 16.9.4 LTS
- TI C6000 Compiler 8.3.3
- XDCtools 3.50.08.24
- SYS/BIOS 6.73.01.01
- mmWave SDK 3.6.2 LTS

For host-sdk/tools:
- Python 3.10+
- ruff, black (installed via pre-commit)

### Building Firmware

```bash
source toolchains/env.sh
cd firmware
make all
```

## Code Style

### Commit Messages

Use simple, descriptive commit messages:

```
fix radar data parsing for negative values
add target auto-selection algorithm
update phase extraction to use atan2 LUT
```

Guidelines:
- Start with lowercase verb: `fix`, `add`, `update`, `remove`, `refactor`
- Keep the first line under 72 characters
- Use imperative mood ("add feature" not "added feature")
- Reference issues when applicable: `fix buffer overflow (#42)`

### Branch Naming

- `feat/` - New features: `feat/phase-output-mode`
- `fix/` - Bug fixes: `fix/uart-overflow`
- `docs/` - Documentation: `docs/api-reference`
- `refactor/` - Code refactoring: `refactor/cli-parser`

### C Code Style

- Use `.clang-format` configuration (auto-applied by pre-commit)
- 120 character line limit
- 4-space indentation (no tabs)
- Allman brace style
- Pointer asterisk on right side: `uint8_t *buffer`

### Python Code Style

- Follows ruff and black formatting (same as ambient project)
- 120 character line limit
- Type hints encouraged

## Pull Request Process

1. Ensure pre-commit hooks pass: `pre-commit run --all-files`
2. Update documentation if adding new features
3. Add tests for new functionality in host-sdk
4. Create PR with clear description of changes
5. Wait for CI checks to pass
6. Request review

### PR Description Template

```markdown
## Summary
Brief description of what this PR does.

## Changes
- List of specific changes made

## Testing
How was this tested?

## Related Issues
Fixes #123
```

## Testing

### Host SDK Tests

```bash
cd host-sdk/python
pytest tests/ -v
```

### Firmware Testing

Firmware changes require hardware testing. Document your test procedure in the PR:
- Configuration used
- Expected vs actual behavior
- Any edge cases tested

## Questions?

Open an issue for discussion before starting major changes.
