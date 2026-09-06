# JrBot - relacao de materiais HW03

Esta relacao cobre o prototipo e a montagem planejada, nao um inventario de compras ja confirmado. Quantidades sao para uma unidade do JrBot. Itens opcionais/futuros nao sao necessarios para operar o painel sem OLED.

| ID | Qtd. | Item | Especificacao / criterio | Situacao |
|---|---:|---|---|---|
| M01 | 1 | Placa ESP32-S3 CAM | Familia N16R8 declarada; boot recebido mostra PSRAM 8 MB e configuracao flash 16 MB | Em uso; fabricante/revisao e esquema pendentes |
| M02 | 1 | Camera e conector integrado | Firmware parametrizado para JPEG e suporte OV5640; confirmar sensor e cabo exatos | Declarada no projeto; teste fisico pendente |
| M03 | 1 | Cabo flex da camera | Passo, numero de vias, comprimento e orientacao compativeis com M01/M02 | Normalmente acompanha camera; confirmar |
| M04 | 1 | OLED I2C 128x64 SSD1306 | Logica do barramento 3,3 V; conferir a alimentacao do breakout | Suspeito de defeito; opcional/desconectado agora |
| M05 | 1 | Amplificador I2S mono MAX98357A | Confirmar variante A, breakout e seus pinos VIN, GND, DIN, BCLK, LRC, SD/MODE e GAIN | Pinagem externa pendente |
| M06 | 1 | Alto-falante passivo | Impedancia/potencia compativeis com M05 e a tensao usada; 4 ou 8 ohms somente se aceitos pelo modulo | Ler etiqueta; nao presumir potencia |
| M07 | 1 | Microfone/modulo | Modelo, interface (I2S/PDM/analogica), tensao e sinais a identificar | Nao configurado; nao comprar substituto no chute |
| M08 | 2 | Cabos USB de dados | Conectores correspondentes as duas USBs da placa; nao apenas carga | COM6 gravacao / COM4 painel nesta montagem |
| M09 | 1 | Computador | Windows com Git, ESP-IDF 5.5.x, Python e navegador | Ja usado no projeto |
| M10 | 1 conjunto | Alimentacao regulada de bancada | Tensoes e corrente conforme placa/modulos; separar ramais de potencia e logica | Dimensionamento pendente |
| M11 | Conforme montagem | Fios/conectores de sinal | 4 para OLED quando recuperado; 3 para I2S; sinais restantes dependem dos modulos | Comprimento curto e identificacao dos dois lados |
| M12 | Conforme montagem | Fios/conectores de alimentacao | Bitola compativel com corrente; retorno GND comum para sinais | Nao usar GPIO como fonte |
| M13 | 0 ou 2 | Pull-ups I2C | Verificar os ja presentes no OLED; valor adequado ao barramento 3,3 V e capacitancia | Nao duplicar resistores sem conferir |
| M14 | Conforme modulo | Capacitores de desacoplamento | Seguir datasheet e placa do fabricante; verificar os ja instalados | Nao ha BOM final de capacitor sem modulo exato |
| M15 | 1 conjunto | Distribuicao/protecao da fonte | Chave, conectores e protecao dimensionados para a alimentacao escolhida | Projeto eletrico pendente |
| M16 | 1 conjunto | Base isolante, suportes e fixacao | Evitar curto no verso da placa, tracao nos fios e no flex | Necessario para montagem permanente |
| M17 | 1 | Multimetro | Verificar continuidade com placa desligada e tensoes com cuidado | Ferramenta de diagnostico |
| M18 | Opcional | Analisador logico/osciloscopio | Entradas compativeis com 3,3 V; ajuda em I2C/I2S | Nao obrigatorio para operar painel |
| M19 | Futuro | Servo(s) e mecanica | Modelo, torque, corrente de pico e limites ainda nao definidos | Sem ligacao nem driver nesta versao |
| M20 | Futuro | Fonte/bateria do conjunto autonomo | Requer projeto de protecao, carga e distribuicao antes do uso | Nao ligar bateria diretamente ao prototipo |
| M21 | Futuro | Servidor/servico de IA | Provedor, custo, privacidade, protocolo e chaves a definir | Sem integracao ativa nesta candidata |

## Nao considerar confirmado

Nao sabemos o modelo do microfone, os destinos de GPIO21/41/42/47, a revisao da placa, a impedancia do falante nem a fonte final. Logo, esta tabela nao autoriza ligar esses itens.

O firmware testa o MAX98357A como **saida**, nao entrada de microfone. O CI aceita 2,5 a 5,5 V segundo o fabricante, mas o breakout e a fonte reais tambem precisam ser verificados. Nao usar uma tensao de sinal de 5 V nos GPIOs do ESP32-S3.

Para fechar a lista de compra, preencher M01, M02, M05, M06, M07 e M10 com foto/modelo e ficha tecnica. Nao comprar outro display ou microfone apenas por um erro de comunicacao.

Fontes: [documentacao tecnica](FONTES.md). Esquema: [ESQUEMA_LIGACAO.md](ESQUEMA_LIGACAO.md).
