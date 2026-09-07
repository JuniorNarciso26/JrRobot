#!/usr/bin/env python3
"""JrBot panel wrapper with an offline browser for repository Markdown docs."""
from __future__ import annotations

import html
import re
import urllib.parse
from pathlib import Path

import app
import stable_panel

DOCS_ROOT = (app.ROOT.parent.parent / "docs").resolve()
_PREVIOUS_DO_GET = app.Handler.do_GET


def _safe_doc(relative: str) -> tuple[Path, str]:
    relative = urllib.parse.unquote(relative or "").replace("\\", "/").lstrip("/")
    if not relative:
        relative = "README.md"
    candidate = (DOCS_ROOT / relative).resolve()
    try:
        rel = candidate.relative_to(DOCS_ROOT)
    except ValueError as exc:
        raise ValueError("Documento fora do diretorio docs") from exc
    if candidate.suffix.lower() != ".md" or not candidate.is_file():
        raise FileNotFoundError(relative)
    return candidate, rel.as_posix()


def _doc_href(target: str, current: str) -> str:
    target = html.unescape(target.strip())
    if target.startswith(("http://", "https://", "mailto:", "#")):
        return target
    parsed = urllib.parse.urlsplit(target)
    path = parsed.path
    if not path:
        return "#" + parsed.fragment if parsed.fragment else "#"
    if path.lower().endswith(".md"):
        base = Path(current).parent
        joined = (base / path).as_posix()
        normalized = Path(joined)
        if ".." in normalized.parts:
            return "#"
        href = "/docs/" + urllib.parse.quote(normalized.as_posix(), safe="/")
        if parsed.fragment:
            href += "#" + urllib.parse.quote(parsed.fragment, safe="-_")
        return href
    return target


def _inline(text: str, current: str) -> str:
    tokens: dict[str, str] = {}

    def token(value: str) -> str:
        key = f"@@JRDOC{len(tokens)}@@"
        tokens[key] = value
        return key

    text = re.sub(
        r"`([^`]+)`",
        lambda m: token("<code>" + html.escape(m.group(1)) + "</code>"),
        text,
    )

    def link(match: re.Match[str]) -> str:
        label = html.escape(match.group(1))
        href = html.escape(_doc_href(match.group(2), current), quote=True)
        external = href.startswith(("http://", "https://"))
        extra = ' target="_blank" rel="noreferrer"' if external else ""
        return token(f'<a href="{href}"{extra}>{label}</a>')

    text = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link, text)
    escaped = html.escape(text)
    escaped = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", escaped)
    escaped = re.sub(r"(?<!\*)\*([^*]+)\*(?!\*)", r"<em>\1</em>", escaped)
    for key, value in tokens.items():
        escaped = escaped.replace(key, value)
    return escaped


def _is_table_separator(line: str) -> bool:
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    return bool(cells) and all(re.fullmatch(r":?-{3,}:?", cell or "") for cell in cells)


def render_markdown(source: str, current: str) -> str:
    lines = source.replace("\r\n", "\n").split("\n")
    out: list[str] = []
    i = 0
    in_code = False
    code_lines: list[str] = []
    code_lang = ""

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if stripped.startswith("```"):
            if not in_code:
                in_code = True
                code_lang = stripped[3:].strip()
                code_lines = []
            else:
                cls = f' class="language-{html.escape(code_lang)}"' if code_lang else ""
                out.append(f"<pre><code{cls}>" + html.escape("\n".join(code_lines)) + "</code></pre>")
                in_code = False
            i += 1
            continue
        if in_code:
            code_lines.append(line)
            i += 1
            continue

        if stripped.startswith("|") and i + 1 < len(lines) and _is_table_separator(lines[i + 1]):
            headers = [c.strip() for c in stripped.strip("|").split("|")]
            i += 2
            rows: list[list[str]] = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")])
                i += 1
            table = ["<div class=\"tablewrap\"><table><thead><tr>"]
            table += ["<th>" + _inline(c, current) + "</th>" for c in headers]
            table.append("</tr></thead><tbody>")
            for row in rows:
                table.append("<tr>" + "".join("<td>" + _inline(c, current) + "</td>" for c in row) + "</tr>")
            table.append("</tbody></table></div>")
            out.append("".join(table))
            continue

        heading = re.match(r"^(#{1,6})\s+(.+)$", stripped)
        if heading:
            level = len(heading.group(1))
            title = heading.group(2)
            slug = re.sub(r"[^a-z0-9_-]+", "-", title.lower()).strip("-")
            out.append(f'<h{level} id="{html.escape(slug)}">{_inline(title, current)}</h{level}>')
            i += 1
            continue

        if stripped.startswith("> "):
            out.append("<blockquote>" + _inline(stripped[2:], current) + "</blockquote>")
            i += 1
            continue

        if stripped in {"---", "***"}:
            out.append("<hr>")
            i += 1
            continue

        if re.match(r"^[-*]\s+", stripped):
            items = []
            while i < len(lines) and re.match(r"^\s*[-*]\s+", lines[i]):
                item = re.sub(r"^\s*[-*]\s+", "", lines[i])
                items.append("<li>" + _inline(item, current) + "</li>")
                i += 1
            out.append("<ul>" + "".join(items) + "</ul>")
            continue

        if re.match(r"^\d+\.\s+", stripped):
            items = []
            while i < len(lines) and re.match(r"^\s*\d+\.\s+", lines[i]):
                item = re.sub(r"^\s*\d+\.\s+", "", lines[i])
                items.append("<li>" + _inline(item, current) + "</li>")
                i += 1
            out.append("<ol>" + "".join(items) + "</ol>")
            continue

        if not stripped:
            i += 1
            continue

        paragraph = [stripped]
        i += 1
        while i < len(lines) and lines[i].strip() and not re.match(
            r"^(#{1,6})\s|^```|^> |^[-*]\s+|^\d+\.\s+|^\|", lines[i].strip()
        ):
            paragraph.append(lines[i].strip())
            i += 1
        out.append("<p>" + _inline(" ".join(paragraph), current) + "</p>")

    if in_code:
        out.append("<pre><code>" + html.escape("\n".join(code_lines)) + "</code></pre>")
    return "\n".join(out)


def _nav(current: str) -> str:
    items = []
    if DOCS_ROOT.is_dir():
        for path in sorted(DOCS_ROOT.rglob("*.md"), key=lambda p: p.relative_to(DOCS_ROOT).as_posix().lower()):
            rel = path.relative_to(DOCS_ROOT).as_posix()
            label = "Início" if rel == "README.md" else rel.replace(".md", "").replace("_", " ")
            active = " active" if rel == current else ""
            href = "/docs/" + urllib.parse.quote(rel, safe="/")
            items.append(f'<a class="doclink{active}" href="{href}">{html.escape(label)}</a>')
    return "".join(items)


def _docs_page(relative: str) -> str:
    path, rel = _safe_doc(relative)
    body = render_markdown(path.read_text(encoding="utf-8"), rel)
    return f"""<!doctype html>
<html lang="pt-br"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>JrBot Docs - {html.escape(rel)}</title>
<style>
:root{{--bg:#0b0d12;--panel:#151923;--line:#252b38;--txt:#eef3ff;--muted:#9aa7bd;--blue:#72a7ff}}
*{{box-sizing:border-box}}body{{margin:0;background:#0b0d12;color:var(--txt);font:15px/1.6 Inter,Segoe UI,Arial,sans-serif}}
a{{color:var(--blue)}}.layout{{display:grid;grid-template-columns:280px minmax(0,1fr);min-height:100vh}}
nav{{border-right:1px solid var(--line);background:#10131a;padding:18px;overflow:auto}}nav h1{{font-size:18px;margin:0 0 8px}}nav p{{color:var(--muted);font-size:12px}}
.doclink{{display:block;padding:7px 9px;border-radius:8px;color:#cbd8ee;text-decoration:none;overflow-wrap:anywhere}}.doclink:hover,.doclink.active{{background:#1c2433;color:white}}
main{{padding:32px;max-width:1000px;width:100%}}h1,h2,h3{{line-height:1.25;margin-top:1.5em}}h1{{margin-top:0}}p,li{{max-width:85ch}}code{{background:#171d29;border:1px solid #283144;border-radius:5px;padding:.1em .35em}}pre{{background:#07090d;border:1px solid var(--line);padding:14px;border-radius:10px;overflow:auto}}pre code{{border:0;padding:0;background:none}}blockquote{{border-left:3px solid #4d79b8;margin-left:0;padding:8px 14px;background:#121824;color:#c8d3e6}}table{{border-collapse:collapse;width:100%}}th,td{{border:1px solid var(--line);padding:8px;text-align:left}}th{{background:#171d29}}.tablewrap{{overflow:auto}}.back{{display:inline-block;margin-bottom:18px}}
@media(max-width:800px){{.layout{{grid-template-columns:1fr}}nav{{border-right:0;border-bottom:1px solid var(--line);max-height:260px}}main{{padding:20px}}}}
</style></head><body><div class="layout"><nav><h1>JrBot Docs</h1><p>Fonte local: <code>docs/</code></p><a class="back" href="/">← Painel</a>{_nav(rel)}</nav><main>{body}</main></div></body></html>"""


def docs_do_get(self) -> None:
    parsed = urllib.parse.urlsplit(self.path)
    if parsed.path == "/":
        if not self._allowed():
            return
        page = (app.ROOT / "index.html").read_text(encoding="utf-8").replace("{APP_VERSION}", app.APP_VERSION)
        docs_link = '<a href="/docs/" style="padding:9px 13px;border:1px solid #3a4c68;border-radius:12px;color:#d9e7ff;text-decoration:none;font-weight:700">Documentação</a>'
        page = page.replace("</header>", docs_link + "</header>")
        page = page.replace("</body>", '<script src="/photo_panel.js"></script></body>')
        self._send(200, page, "text/html; charset=utf-8")
        return
    if parsed.path in {"/docs", "/docs/"}:
        if not self._allowed():
            return
        try:
            self._send(200, _docs_page("README.md"), "text/html; charset=utf-8")
        except (ValueError, FileNotFoundError, OSError):
            self._send(404, "Documentacao nao encontrada")
        return
    if parsed.path.startswith("/docs/"):
        if not self._allowed():
            return
        try:
            self._send(200, _docs_page(parsed.path[len("/docs/"):]), "text/html; charset=utf-8")
        except (ValueError, FileNotFoundError, OSError):
            self._send(404, "Documento nao encontrado")
        return
    _PREVIOUS_DO_GET(self)


app.Handler.do_GET = docs_do_get
app.APP_VERSION = "JRBOT-PANEL-V2-13-DOCS"


if __name__ == "__main__":
    raise SystemExit(stable_panel.main())
