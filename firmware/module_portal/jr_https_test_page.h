#pragma once

static const char JR_HTTPS_TEST_HTML[] =
"<!doctype html><html lang='pt-br'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>JrBot HTTPS EXP</title><style>body{font:16px system-ui;background:#0b1020;color:#eef4ff;margin:0;padding:20px}main{max-width:720px;margin:auto}.card{background:#151d31;border:1px solid #30405f;border-radius:16px;padding:16px;margin:12px 0}button{background:#243454;color:white;border:1px solid #48628f;border-radius:10px;padding:12px 16px;font:inherit}pre{white-space:pre-wrap;word-break:break-word;background:#090e19;padding:12px;border-radius:10px;min-height:40px}audio{width:100%;margin-top:12px}.ok{color:#72e6a6}.bad{color:#ff9090}.warn{color:#ffd77a}</style></head><body><main>"
"<h1>JrBot HTTPS EXP 03</h1><div class='card'><b>Diagnostico do navegador</b><pre id='diag' class='warn'>Aguardando /https-test.js...</pre><button id='refresh'>Atualizar diagnostico</button></div>"
"<div class='card'><b>Teste do microfone</b><p id='state'>Ainda nao testado.</p><button id='mic'>Gravar 3 segundos</button><audio id='preview' controls hidden></audio><pre id='err'>Sem erro registrado.</pre></div>"
"<div class='card'>Objetivo: certificado confiavel + <b>Secure Context = SIM</b> + <b>getUserMedia = SIM</b>. O diagnostico agora e carregado por um arquivo JavaScript separado para identificar claramente falha de execucao/carregamento.</div>"
"<script src='/https-test.js' defer></script></main></body></html>";
