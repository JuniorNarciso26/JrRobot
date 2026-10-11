# Testes e validação

O JrBot é um projeto físico. Compilar código não prova comportamento real do robô.

## Níveis de evidência

### 1. Planejado

Existe apenas design/documentação.

### 2. Implementado

Código foi escrito, mas ainda não há comprovação de build ou hardware.

### 3. Build verificado

O projeto compila/empacota para o alvo correto.

### 4. Bancada ou simulado

O caminho lógico foi exercitado por simulação, comando de diagnóstico ou mock.

### 5. Validado fisicamente

A função foi observada no hardware real.

## Exemplos

### Reconhecimento de voz

`brain_test` simulando wakeword prova integração do cérebro/rosto, mas não reconhecimento acústico.

Um log com:

```text
name detected ... probability=...
```

durante fala real pode sustentar validação acústica daquele teste.

### Áudio

```text
PLAY_RECORDING_END result=ESP_OK
```

prova conclusão do caminho de software/I2S. Para afirmar “som audível”, registre confirmação humana no teste.

### Câmera

JPEG válido prova captura digital. Não prova foco, exposição adequada ou qualidade visual.

## Checklist de candidata de voz

1. confirmar versão da placa;
2. confirmar modelo carregado;
3. confirmar microfone aberto;
4. realizar várias chamadas reais;
5. registrar probabilidades;
6. falar frases não relacionadas;
7. testar ruído/TV/silêncio quando relevante;
8. verificar resposta de áudio;
9. verificar retomada de escuta;
10. procurar reboot/assert;
11. salvar log completo.

## Calibração do Playground

Uma sessão de calibração deve separar:

- tentativas positivas: usuário realmente falou a frase alvo;
- tentativas negativas: outras falas/ruídos;
- candidatos recebidos;
- confiança;
- detecções aceitas pela configuração selecionada.

A escolha de threshold precisa equilibrar falsos positivos e falsos negativos com dados do hardware real.

## Critério de projeto vs garantia do modelo

Metas internas como “8/10 detecções e 0/20 falsos positivos” podem ser usadas como critério de aceitação do projeto, mas devem ser identificadas como metas definidas pelo JrBot, não garantias do ESP-SR.

## Relatório mínimo de teste

```text
Firmware:
Hardware:
Branch/commit:
Data:
Ambiente:
Procedimento:
Resultado esperado:
Resultado observado:
Logs:
Validação física:
Problemas encontrados:
```
