import { mkdirSync, writeFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { connect, readVerified, PublicKey } from './client.mjs';
import { loadPayload } from './payload.mjs';

try {
  if (!process.argv[2]) throw new Error('Usage: npm run read:devnet -- <publisher-authority-pubkey> [skill-json-path]');
  const authority = new PublicKey(process.argv[2]);
  const source = process.argv[3] || process.env.JRSKILL_FILE || undefined;
  const { program } = await connect({ readOnly: true });
  const original = loadPayload(source);
  const { pda, recovered } = await readVerified(program, authority, original);
  const output = resolve('artifacts', `${pda.toBase58()}.json`);
  mkdirSync(resolve('artifacts'), { recursive: true });
  writeFileSync(output, recovered);
  console.log(JSON.stringify({
    result: 'ok',
    cluster: 'devnet',
    program: program.programId.toBase58(),
    authority: authority.toBase58(),
    pda: pda.toBase58(),
    payload_bytes: recovered.length,
    payload_hash: original.payloadHash.toString('hex'),
    byte_equal: true,
    output,
    source: original.source,
  }));
} catch (error) {
  console.error(error);
  process.exitCode = 1;
}
