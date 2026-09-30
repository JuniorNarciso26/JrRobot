import { connect, deriveSkill, createInstruction, readVerified, Transaction } from './client.mjs';
import { loadPayload } from './payload.mjs';

try {
  const original = loadPayload();
  const { program, provider, connection } = await connect();
  const authority = provider.wallet.publicKey;
  const pda = deriveSkill(program.programId, authority, original.payloadHash);
  let signature = null;
  if (await connection.getAccountInfo(pda)) {
    await readVerified(program, authority, original); // Repeat is read-only; never overwrite.
  } else {
    const instruction = await createInstruction(program, authority, original);
    const transaction = new Transaction().add(instruction);
    transaction.feePayer = authority;
    transaction.recentBlockhash = (await connection.getLatestBlockhash()).blockhash;
    const size = transaction.serialize({ requireAllSignatures: false, verifySignatures: false }).length;
    if (size > 1232) throw new Error(`Transaction exceeds 1232 bytes: ${size}`);
    console.log(JSON.stringify({ transaction_bytes: size, account_bytes: 589, rent_lamports: await connection.getMinimumBalanceForRentExemption(589) }));
    signature = await provider.sendAndConfirm(transaction);
    await readVerified(program, authority, original);
  }
  console.log(JSON.stringify({ result: 'ok', cluster: 'devnet', program: program.programId.toBase58(), authority: authority.toBase58(), pda: pda.toBase58(), payload_bytes: original.payload.length, payload_hash: original.payloadHash.toString('hex'), signature, already_exists: signature === null }));
} catch (error) {
  console.error(error);
  process.exitCode = 1;
}
