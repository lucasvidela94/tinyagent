# Documentation inventory and suggestions

## Included now

- `../README.md`: quick start and build/use instructions.
- `../AGENTS.md`: persistent project instructions for coding agents.
- `GOALS.md`: goal, non-goals and definition of done.
- `ARCHITECTURE.md`: architecture diagram, data flow and code map.
- `ACCESS_POINTS.md`: entry points, prompts, shell/network interfaces and permissions.
- `PC_PROFILE.md`: target machine facts and implications.
- `OPERATIONS.md`: operational runbook and safe initial diagnostics.
- `SECURITY.md`: trust boundaries, secret handling and limitations.
- `API.md`: API contract and failure modes.
- `TEST_PLAN.md`: build, integration, safety and acceptance checks.
- `ROADMAP.md`: prioritized implementation plan.

## Add later when the implementation grows

1. `CONTRIBUTING.md`: formatting, compiler matrix, patch workflow and review checklist.
2. `THREAT_MODEL.md`: assets, attackers, abuse cases and mitigations if the agent gains file-write tools.
3. `DECISIONS/ADR-0001-target-x86.md`: why x86 native C and remote inference were selected.
4. `API_COMPATIBILITY.md`: dated sample request/response fixtures verified against OpenCode Go.
5. `RELEASE.md`: versioning, reproducible build steps, SHA-256 checksum and release notes.
6. `TEST_FIXTURES/`: sanitized API JSON fixtures and parser tests.
7. `PRIVACY.md`: exactly what is sent to the model, retention assumptions and redaction guidance.

Avoid writing documents that promise features not implemented. Update the architecture and test plan whenever a new tool, API, permission or storage mechanism is introduced.
