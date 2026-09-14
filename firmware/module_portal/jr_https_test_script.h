#pragma once

static const char JR_HTTPS_TEST_JS[] =
"(function(){"
"var q=function(id){return document.getElementById(id)};"
"function yn(v){return v?'SIM':'NAO'}"
"function setText(id,text,cls){var e=q(id);if(!e)return;e.textContent=text;if(cls)e.className=cls}"
"function diag(){try{var md=!!navigator.mediaDevices;var gm=!!(navigator.mediaDevices&&navigator.mediaDevices.getUserMedia);var mr=!!window.MediaRecorder;var ac=!!(window.AudioContext||window.webkitAudioContext);var lines=['JavaScript externo: SIM','URL: '+location.href,'Protocolo: '+location.protocol,'Secure Context: '+yn(window.isSecureContext),'mediaDevices: '+yn(md),'getUserMedia: '+yn(gm),'MediaRecorder: '+yn(mr),'AudioContext: '+yn(ac),'User agent: '+navigator.userAgent];setText('diag',lines.join('\\n'),window.isSecureContext&&gm&&mr?'ok':'bad');}catch(e){setText('diag','Falha no diagnostico: '+String(e),'bad')}}"
"function stop(s){if(s&&s.getTracks){var t=s.getTracks();for(var i=0;i<t.length;i++)t[i].stop()}}"
"function testMic(){diag();setText('err','Sem erro registrado.');var preview=q('preview');if(preview)preview.hidden=true;if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia){setText('state','getUserMedia indisponivel. Verifique o certificado/Secure Context.','bad');return}if(!window.MediaRecorder){setText('state','MediaRecorder indisponivel.','bad');return}setText('state','Solicitando permissao...');navigator.mediaDevices.getUserMedia({audio:true,video:false}).then(function(stream){var chunks=[];var rec=new MediaRecorder(stream);rec.ondataavailable=function(e){if(e.data&&e.data.size)chunks.push(e.data)};rec.onerror=function(e){stop(stream);setText('state','Falha durante a gravacao.','bad');setText('err','MediaRecorder: '+String(e&&e.error?e.error:e),'bad')};rec.onstop=function(){var type=rec.mimeType||'audio/webm';var blob=new Blob(chunks,{type:type});stop(stream);if(preview){preview.src=URL.createObjectURL(blob);preview.hidden=false}setText('state','Gravacao concluida. Ouça o preview.','ok');diag()};rec.start();setText('state','Gravando por 3 segundos...','ok');setTimeout(function(){if(rec.state==='recording')rec.stop()},3000)}).catch(function(e){setText('state','Falha ao acessar o microfone.','bad');setText('err','Erro: '+(e&&e.name?e.name+': ':'')+(e&&e.message?e.message:String(e)),'bad');diag()})}"
"function start(){var r=q('refresh'),m=q('mic');if(r)r.onclick=diag;if(m)m.onclick=testMic;diag()}"
"if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',start);else start();"
"})();";
