# JrBot — revisão de Issues abertas (26/10/2026)

Registro de triagem dos três repositórios. **Uma Issue aberta não significa que toda a funcionalidade descrita esteja ausente.** Não encerrar roadmap ou defeito não corrigido apenas para reduzir a contagem.

## Aceite da versão atual

- Firmware em `main`: `JrBot_V1.8.1S`.
- Testes físicos: **OK**, declaração expressa do responsável em 26/10/2026.
- Testes de memória: **concluídos**, mesma confirmação; novas medições não são exigidas para este marco.
- A documentação não inclui logs inéditos ou novas métricas não fornecidas.
- **#55 Wi-Fi pelo celular: closed/completed** após registro da integração e aceite. Histórico e guia preservados.

## Prioridade A — saneamento de histórico / correção

| Issue | Direção | Não assumir sem evidência |
| --- | --- | --- |
| [JrRobot #14 — V1 local](https://github.com/JuniorNarciso26/JrRobot/issues/14) | **Revisada em 26/10:** painel, foto, WAV enviado do celular, DHCP e Live local já implementados. **Manter aberta** por gravação/exportação de vídeo curto, contrato de 4 estados do microfone, assets nomeados e itens antigos ainda não comprovados. [Auditoria na Issue](https://github.com/JuniorNarciso26/JrRobot/issues/14#issuecomment-6420896384) | Não confundir Live com gravação em arquivo; testes físicos/memória gerais estão concluídos |
| [JrRobot #26 — WebRTC](https://github.com/JuniorNarciso26/JrRobot/issues/26) | **Revisada em 26/10:** Live local (JPEG + áudio bidirecional) concluído; Issue continua aberta exclusivamente para Internet e qualidade (AEC/jitter), com chiado em #19. [Registro atualizado](https://github.com/JuniorNarciso26/JrRobot/issues/26#issuecomment-6420919719) | Internet e cancelamento de eco não são recursos entregues |
| [JrRobot #19 — MAX98357A](https://github.com/JuniorNarciso26/JrRobot/issues/19) | Diagnosticar chiado na reprodução antes de encerramento | Problema corrigido pela aceitação geral do firmware |
| [JrRobot #9 — licença](https://github.com/JuniorNarciso26/JrRobot/issues/9) | Conferir condições da licença MIT atual e eventual decisão pendente; revisar como issue histórica | Questões comerciais e de marca integralmente resolvidas |
| [JrBot_Web #30 — site comercial](https://github.com/JuniorNarciso26/JrBot_Web/issues/30) | Conferir o que já foi integrado em main e o que falta em deploy/Hostinger/checkout/WhatsApp | Código integrado significa campanha publicada ou pagamento ativo |

## Prioridade B — trabalho funcional ainda aberto

| Issue | Tema |
| --- | --- |
| [JrBot_Cloud #1](https://github.com/JuniorNarciso26/JrBot_Cloud/issues/1) | Conectividade segura pela Internet, separada do Live local |
| [JrBot_Web #16](https://github.com/JuniorNarciso26/JrBot_Web/issues/16) | Store / maturidade comercial |
| [JrBot_Cloud #3](https://github.com/JuniorNarciso26/JrBot_Cloud/issues/3) | API Wallet/Offer/License e publicação |
| [JrRobot #15](https://github.com/JuniorNarciso26/JrRobot/issues/15) | Voz local V2; PR #21 permanece em rascunho |
| [JrRobot #16](https://github.com/JuniorNarciso26/JrRobot/issues/16) | V3 Runtime API |
| [JrRobot #11](https://github.com/JuniorNarciso26/JrRobot/issues/11) | JrBrain via LLM API |
| [JrRobot #12](https://github.com/JuniorNarciso26/JrRobot/issues/12) | JrTalk local |
| [JrRobot #53](https://github.com/JuniorNarciso26/JrRobot/issues/53) | Piloto externo JrSkill |

## Prioridade C — planejamento, estratégia e melhorias futuras

- [JrRobot #17](https://github.com/JuniorNarciso26/JrRobot/issues/17): epic de roadmap geral.
- [JrRobot #10](https://github.com/JuniorNarciso26/JrRobot/issues/10): epic JrBrain.
- [JrRobot #8](https://github.com/JuniorNarciso26/JrRobot/issues/8): bounties e Contributor Pool.
- [JrRobot #22](https://github.com/JuniorNarciso26/JrRobot/issues/22): corpo e impressão 3D.
- [JrRobot #24](https://github.com/JuniorNarciso26/JrRobot/issues/24): estéreo.
- [JrRobot #28](https://github.com/JuniorNarciso26/JrRobot/issues/28): PIPE / Remote Expert; verificar prazo histórico antes de decidir ação.
- [JrRobot #52](https://github.com/JuniorNarciso26/JrRobot/issues/52): tração do hackathon; revisar propósito após MVP.
- [JrRobot #56](https://github.com/JuniorNarciso26/JrRobot/issues/56): NFC, bateria, boost.

## Pull Requests ainda abertos

- [JrRobot PR #21](https://github.com/JuniorNarciso26/JrRobot/pull/21): V2 voz local, draft; não mesclar como parte desta limpeza.
- [JrBot_Cloud PR #4](https://github.com/JuniorNarciso26/JrBot_Cloud/pull/4): JrSkill Studio API; revisar integração/validação antes de merge.

## Sequência de tratamento

1. Concluído: #55 Wi-Fi, documentação da aceitação.
2. #14 V1 revisada: manter aberta somente para lacunas reais identificadas, sem refazer funções prontas.
3. #26 WebRTC revisada: Live local concluído; Internet e AEC continuam como evolução futura.
4. Próxima: #19 chiado: tratar defeito isolado, sem atribuir o problema automaticamente ao WebRTC.
5. #9 licença e Web #30: revisão documental e operacional.
6. Demais Issues: manter abertas enquanto houver requisito futuro real; documentar decisão e motivo de cada fechamento individual.

**Regra de fechamento:** concluir apenas escopo objetivamente implementado ou decisão explícita de não prosseguir; manter histórico e links; registrar mudanças de estado. Não executar alterações no firmware, releases, PRs ou serviços como efeito da triagem.
