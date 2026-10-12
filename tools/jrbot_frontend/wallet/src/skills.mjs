const GENESIS = 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG';
const PROGRAM = 'Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454';
export function createSkillSearch(request, render, log = () => {}) {
  let address = '', provider, authenticated = false, pending = false, epoch = 0, skills = [];
  let message = 'Conecte e autentique a carteira para buscar.';
  function emit() { render({ authenticated, pending, skills, message }); }
  const api = {
    observe(state, auth) {
      const next = state.address || '';
      const ready = !!next && !!auth.authenticated && auth.address === next && !auth.pending && !state.pending;
      if (next !== address || ready !== authenticated || state.active !== provider) {
        epoch++; address = next; provider = state.active; authenticated = ready;
        pending = false; skills = []; message = 'Conecte e autentique a carteira para buscar.';
        emit();
        if (ready) void api.search();
      } else emit();
    },
    async search() {
      if (!authenticated || pending) return;
      const attempt = epoch, expected = address;
      pending = true; skills = []; message = 'Buscando suas licencas na Devnet...'; emit();
      try {
        const result = await request();
        if (attempt !== epoch) return;
        if (result.cluster !== 'devnet' || result.genesis !== GENESIS || result.program !== PROGRAM ||
            result.buyer !== expected || result.commitment !== 'finalized' ||
            !Number.isSafeInteger(result.rpc_slot) || result.rpc_slot < 0 || !Array.isArray(result.skills) || result.skills.length > 32)
          throw new Error('Resposta da busca invalida');
        const keys = new Set();
        for (const item of result.skills) {
          if (!item || !/^[1-9A-HJ-NP-Za-km-z]{32,44}$/.test(item.skill) ||
              !/^[1-9A-HJ-NP-Za-km-z]{32,44}$/.test(item.license) || keys.has(item.skill) ||
              item.model_version !== 0 || item.schema_version !== 1 || item.hash_verified !== true ||
              !/^[a-f0-9]{64}$/.test(item.payload_hash) || typeof item.name !== 'string' || item.name.length > 64 ||
              typeof item.checkpoint_supported !== 'boolean') throw new Error('Registro de Skill invalido');
          keys.add(item.skill);
        }
        skills = result.skills;
        message = skills.length ? skills.length + ' Skill(s) licenciada(s) encontrada(s).' : 'Esta carteira nao possui Skills licenciadas neste programa na Devnet.';
        log('JR_SKILL_DISCOVERY result=ok buyer=' + expected + ' count=' + skills.length + ' rpc_slot=' + result.rpc_slot);
      } catch (error) {
        if (attempt !== epoch) return;
        skills = []; message = 'Busca nao confirmada: ' + error.message;
        log('JR_SKILL_DISCOVERY result=error buyer=' + expected + ' detail=' + error.message);
      } finally { if (attempt === epoch) { pending = false; emit(); } }
    }
  };
  emit();
  return api;
}
