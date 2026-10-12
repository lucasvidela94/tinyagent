# Remote API integration

TinyShell speaks the OpenAI-compatible chat-completions format to whatever `url` is set in `config.ini` (see `docs/ACCESS_POINTS.md`). The defaults below are for OpenCode Go.

## Request

- `POST <url>` (default `https://opencode.ai/zen/go/v1/chat/completions`)
- Header: `Authorization: Bearer <API key>` (omitted when no key is set, e.g. a local server)
- Header: `Content-Type: application/json`
- Header `<session_header>: tinyshell-<32 hex>`: one random ID per conversation (new on `/reset`), only if `session_header` is set. Default `x-opencode-session`, **required by OpenCode Go** (HTTP 400 `MissingSessionID` without it); leave it empty for other providers.
- User-Agent: `TinyShell/<version>` (OpenCode asks clients to identify themselves).
- Model field: `model` from `config.ini` (default `deepseek-v4.1-flash`)
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
