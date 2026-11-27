# Conexão direta — teste no JrBot HW04

Branch: `test/wifi-direct`, derivada da `main` V2.1 (`cb3c0048`).
Firmware: `JrBot_V2.1_WIFI_DIRECT_TEST`.

O celular conecta ao ponto de acesso (SoftAP) do robô, sem roteador nem internet.
“Conexão direta” é o nome no produto; não usa o protocolo P2P Wi-Fi Direct.
A aplicação completa é preservada, incluindo voz. O instalador usa ESP-IDF 6.1.1,
vTTS/HTTPS e build separado `b-wifi-direct-vtts`.

## Instalar e ativar

1. Execute `INSTALAR.bat` no Windows, escolha **test/wifi-direct** e a porta COM.
   Aguarde compilação/gravação. Confirme versão e Git no log de boot.
2. Se já conectar ao roteador, abra o painel HTTPS e a aba **Wi-Fi**.
   Se não houver rede, conecte o celular a **JrBot-Setup-XXXXXX** e abra
   `http://192.168.4.1`. Com o robô ocioso, também é possível segurar **BOOT
   por 3 segundos e soltar** para abrir essa configuração.
3. Em **Conexão direta**, escolha uma senha de 8 a 63 caracteres ASCII,
   sem acentos. Ative e confirme o reinício.
4. A rede de configuração será encerrada. No Wi-Fi do celular, selecione
   **JrBot-XXXXXX** e informe a senha escolhida. O sufixo distingue cada robô.
   Se o celular avisar “sem internet”, escolha manter a conexão.
5. Abra **Safari ou Chrome**, fora da janela automática de configuração,
   e digite **https://192.168.4.1**. O certificado local é autoassinado;
   pode ser necessário aceitar o aviso de certificado para o endereço do robô.
   `https://jrbot.local` também é anunciado por mDNS no AP, mas o IP é a
   referência do teste. Nenhum serviço externo é necessário para abrir o painel.
6. A escolha fica salva e o próximo boot mantém a rede própria do robô.

## Voltar ao Wi-Fi normal

Pare Live e modo autônomo. Na aba **Wi-Fi**, desative **Conexão direta** e
confirme o reinício. O robô volta a tentar as redes salvas, que não são apagadas.
Reconecte o celular ao roteador e abra o painel no endereço habitual.
Sem rede disponível, a recuperação existente abre o JrBot-Setup.

Se esquecer a senha da conexão direta, use **BOOT por 3 segundos e solte**
com o robô ligado e ocioso. O portal de configuração tem prioridade sobre o
modo direto: é possível definir outra senha ou testar/salvar uma rede normal.
Salvar uma rede normal com sucesso nesse portal desativa a conexão direta
no próximo boot. Também é possível reinstalar `main`, que ignora essa opção.

## Escopo e comportamento

- SoftAP somente, canal 1, WPA2-PSK, até dois clientes. Não mantém uma estação
  procurando roteadores em paralelo; não há tarefa adicional de varredura.
- Usa a interface AP padrão do ESP-IDF, DHCP e IP `192.168.4.1`.
- Os servidores normais HTTP/HTTPS e suas rotas são iniciados nesse modo;
  o pequeno servidor de configuração/DNS cativo não roda simultaneamente.
- A aba Wi-Fi informa “Conexão direta ativa”; não inventa RSSI de roteador.
  `connected` na API continua significando associação STA ao roteador, enquanto
  `network_ready` também inclui o AP disponível. O campo `direct` distingue os modos.
- Busca de redes fica indisponível no modo direto. Para buscar, use BOOT ou
  desative o modo; é possível consultar e preparar os perfis salvos no painel.
  Para aplicar um perfil, desative a conexão direta.
- Ativação/desativação pelo painel exige modo ocioso e reinicia após 2,5 s.
  O portal inicial também bloqueia ativação durante o teste de rede/reinício.
- Senha escolhida pelo usuário fica em NVS local, separada das redes do roteador;
  não é retornada pelo status nem registrada nos logs. Nenhuma senha padrão
  compartilhada foi acrescentada. O painel usa a proteção WPA2 da rede e mantém
  o comportamento de autenticação local da versão base.
- Não oferece internet ao celular, NAT, acesso Cloud ou Live remoto.
  A manutenção simultânea dos dados móveis depende do celular.

## Roteiro de validação física

1. Entrar no AP de configuração e ativar o modo direto.
2. Conectar ao novo SSID com senha correta; conferir rejeição de senha incorreta.
3. Abrir o painel pelo IP, consultar estado, trocar a face e testar uma foto.
4. Na aba Voz, testar uma frase curta. Depois testar “Olá ESP” com Autônomo.
5. Com Autônomo desligado, testar Live: vídeo, áudio nos dois sentidos,
   encerramento e reconexão. Permitir microfone no navegador HTTPS.
6. Desligar/ligar o robô e verificar que a conexão direta permanece selecionada.
7. Desativar a conexão direta e verificar retorno ao roteador e à Live local.
8. Ativar novamente, usar BOOT para recuperar a configuração e salvar uma rede
   normal; verificar que inicia nessa rede após reinício.

Enviar o log desde o boot e informar o celular/navegador e os itens que passaram.
No Serial, procurar `JR_WIFI_DIRECT_READY`, `JR_HTTPS_READY`,
`JR_MDNS_READY ... interface=AP` e, em caso de erro, `JR_WIFI_DIRECT_ERROR`.

## Validação realizada nesta entrega

- Compilação/execução em host do módulo C real: validação de senhas, persistência
  simulada, aplicação apenas após reinício, AP WPA2 e limpeza em 11 pontos de falha.
- Execução em host do `app_main` real: rede normal, modo direto sem roteador,
  prioridade do BOOT, configuração inicial e recuperação após falha do AP.
- Testes existentes de perfis Wi-Fi, recuperação, scan, mDNS (agora também AP),
  instalação e voz aprovados; política de pinos HW04 preservada.
- JavaScript da aba Wi-Fi e da configuração inicial com sintaxe verificada.

**Pendente:** este ambiente não possui toolchain ESP-IDF/Xtensa. A compilação
completa será executada pelo instalador no PC. Ainda não houve gravação física,
medição de RAM dinâmica, conexão iOS/Android ou validação de câmera/Live nesse
modo. Os testes host não comprovam desempenho de rádio, mídia ou permissões do
navegador. O código da `main` não foi alterado.
