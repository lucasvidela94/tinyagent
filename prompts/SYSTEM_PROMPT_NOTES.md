# System prompt notes

The first version embeds its system prompt in `src/main.c` to avoid a runtime file dependency. The current prompt instructs the model to:

- Respond in Spanish and keep advice practical.
- Diagnose first using read-only commands.
- Explain the reason for each action.
- Avoid deletes, formatting, antivirus/firewall disabling and unverified repairs.
- Recommend backing up important files before any repair that writes to disk, registry or services (SECURITY rule 5).
- Avoid `&` chaining, `>` redirection and interactive commands (commands run with stdin = NUL and a time limit).
- Verify results rather than assuming success.

If the prompt is changed, keep the local approval gate in code. Prompt instructions alone are not a security control: the executable must enforce safety locally.
