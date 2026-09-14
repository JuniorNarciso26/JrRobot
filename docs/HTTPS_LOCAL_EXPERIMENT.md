# Experimento HTTPS local

Branch: `experiment/https-local`

Identificacao experimental: `JrBot_HTTPS_EXP_01`

## Objetivo

Validar, sem alterar a linha `v1`, se um painel HTTPS servido diretamente pelo ESP32-S3 consegue liberar as APIs de microfone do navegador em celular e computador e manter o envio do audio para o JrBot.

## Regras

- A branch `v1` nao recebe alteracoes durante o experimento.
- O HTTP atual deve continuar disponivel durante os primeiros testes para recuperacao e comparacao.
- Nenhuma chave privada de certificado deve ser commitada no repositorio publico.
- Certificados/chaves locais de teste ficam em `firmware/certs/local/`, caminho ignorado pelo Git.
- O experimento nao altera pinagem nem permite controle direto de GPIO pelo painel.

## Etapa 1 - HTTPS minimo

Adicionar `esp_https_server` em paralelo ao HTTP atual e carregar certificado/chave de teste gerados localmente.

Validar:

- servidor HTTPS inicia;
- painel abre por `https://...`;
- HTTP atual continua acessivel;
- consumo de RAM/PSRAM e estabilidade permanecem aceitaveis.

## Etapa 2 - diagnostico do navegador

Exibir no painel experimental:

- `location.protocol`;
- `window.isSecureContext`;
- disponibilidade de `navigator.mediaDevices`;
- disponibilidade de `getUserMedia`;
- disponibilidade de `MediaRecorder`;
- disponibilidade de `AudioContext`;
- erro real retornado ao solicitar permissao do microfone.

## Etapa 3 - gravacao

Fluxo de teste:

1. tocar em `Gravar`;
2. conceder permissao ao microfone;
3. gravar ate 10 segundos;
4. parar;
5. ouvir o preview local;
6. converter para WAV PCM 16-bit mono 16 kHz;
7. enviar para `/audio`;
8. reproduzir no JrBot.

## Matriz de teste

- PC + Chrome/Edge;
- Android + Chrome;
- iPhone + Safari.

Para cada ambiente registrar:

- certificado aceito/confiavel;
- `isSecureContext`;
- `getUserMedia` disponivel;
- permissao do microfone;
- gravacao concluida;
- preview local;
- envio para o JrBot;
- reproducao fisica.

## Criterio para voltar para V1

Somente integrar na `v1` quando o caminho HTTPS estiver fisicamente validado e tivermos definido o comportamento de certificado para os navegadores alvo. A integracao deve trazer apenas as alteracoes comprovadas, mantendo o restante da `JrBot_V1.4.3` intacto ate a proxima revisao oficial.
