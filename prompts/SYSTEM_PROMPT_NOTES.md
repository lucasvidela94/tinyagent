# System prompt notes

The first version embeds its system prompt in `agent.c` to avoid a runtime file dependency. The current prompt instructs the model to:

- Respond in Spanish and keep advice practical.
- Diagnose first using read-only commands.
- Explain the reason for each action.
- Avoid deletes, formatting, antivirus/firewall disabling and unverified repairs.
- Verify results rather than assuming success.

If the prompt is changed, keep the local approval gate in code. Prompt instructions alone are not a security control: the executable must enforce safety locally.
