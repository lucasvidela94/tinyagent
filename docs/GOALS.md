# Goals and non-goals

## Goal

A small, general-purpose terminal agent (in the spirit of OpenCode or Pi) for old PCs, Windows or Linux: diagnosing problems, inspecting files, writing scripts, maintenance, or answering questions about the machine. The model reasons remotely; the old machine only keeps the console, speaks HTTPS and runs the commands the user approves. The main test machine runs Windows 7 x86.

## Goals

1. One 32-bit executable per platform that runs on the target machines with low local resource use (Windows: PE32 linking only system DLLs; Linux: static i586 binary).
2. No Node.js, Go, Python, database or background service needed to run the binary.
3. Talk to any OpenAI-compatible chat-completions API, configured in `config.ini` (default: OpenCode Go with `deepseek-v4.1-flash`).
4. Show the exact command and its reason before running it.
5. Inspect first; changes only after a deliberate approval.
6. Give the output and exit code back to the model so it iterates on real evidence.
7. Keep the project easy to build, inspect and change.
8. Keep the system prompt task- and machine-agnostic: the machine is detected at startup.

## Non-goals (for now)

- Running tasks unattended or elevating privileges automatically.
- Replacing an antivirus, a recovery tool, a technician or a backup.
- Running an LLM locally on 2 GB of RAM.
- Being a full Pi/OpenCode clone: no codebase navigation, built-in editing, git tools, MCP, persistent shell or advanced planning.
- Guaranteeing a correct repair in every case.

## Definition of done for the first usable build

- Builds as PE32 x86 and identifies as 32-bit.
- Starts on Windows 7 SP1 x86 without installing extra runtimes.
- A harmless diagnostic request completes the round trip with the API.
- The answer can propose a tool call; rejecting it prevents any execution.
- Approving a read-only command returns its output and result to the model.
- The API key is never written to disk unasked or printed in logs.
- Network and API errors are shown without the program closing unexpectedly.
