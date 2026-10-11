# Remote API integration

## Request

- `POST https://opencode.ai/zen/go/v1/chat/completions`
- Header: `Authorization: Bearer <API key>`
- Header: `Content-Type: application/json`
- Model field: `deepseek-v4.1-flash`
- Chat messages include a Spanish system instruction and session history.
- Declares one function tool named `shell` with `command` and `reason` string arguments.

## Expected interaction

1. Assistant returns text, or an assistant message with a `tool_calls` entry.
2. Local app validates the tool name and asks for local approval.
3. Local app sends a `tool` result with `tool_call_id` and captured command output.
4. The model continues or returns a final answer.

## Failure cases to test

- DNS/connect timeout/TLS handshake failure.
- HTTP 401/403, rate limiting, server errors.
- Unsupported model or tool calling.
- Empty/malformed JSON, no `choices`, no `message`, null content.
- Tool arguments returned as a JSON string versus object.
- Very large command output and many tool calls.

## References

- OpenCode Go docs: <https://docs.opencode.ai/docs/go/>

Do not assume compatibility based solely on the URL. Verify with a harmless live request; API implementations can differ in tool-call details and error formats.
