# Roadmap

## P0 — required before real use

- [ ] Cross-compile as PE32 x86 and inspect binary architecture.
- [ ] Test start-up and WinHTTP TLS on the actual Windows 7 SP1 machine.
- [ ] Validate a harmless chat-completions request with the selected API/model.
- [ ] Validate one tool-call round trip with `ver` or `whoami`.
- [ ] Fix any API schema/parser issues revealed by real responses.
- [ ] Verify rejection always means zero local command execution.
- [ ] Add bounded history and response limits.

## P1 — safety and robustness

- [ ] Add explicit read-only mode and default-deny command allowlist.
- [ ] Replace shell-string execution with safer process creation where feasible.
- [ ] Add robust HTTP status/error reporting without leaking credentials.
- [ ] Unit-test JSON escaping, token boundaries, missing fields and malformed responses.
- [ ] Add session reset command and history trimming.
- [ ] Check command exit status and capture stdout/stderr separately if feasible.

## P2 — useful harness features

- [ ] Optional diagnostic bundles with user review before sharing.
- [ ] Read-only file inspection with explicit path approval.
- [ ] Carefully scoped file edits using before/after diff and backup.
- [ ] Small local project/context file describing the machine and allowed actions.
- [ ] Configuration for endpoint/model, with secrets kept outside config.
- [ ] Repeatable CI cross-build and artifact checksum.

## Explicitly out of scope for the low-spec target

- Local model inference, Electron, browser UI, Docker, persistent agents and heavyweight runtimes.
