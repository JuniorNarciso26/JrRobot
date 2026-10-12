import { createRequire } from 'node:module';
import { readFileSync } from 'node:fs';
import { homedir } from 'node:os';
import { resolve } from 'node:path';
import { loadPayload, verifySkill } from './payload.mjs';

const require = createRequire(import.meta.url);
const anchor = require('@anchor-lang/core');
export const { PublicKey, SystemProgram, Transaction } = anchor.web3;
export const DEVNET_URL = 'https://api.devnet.solana.com';
export const DEVNET_GENESIS = 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG';

export function assertDevnetGenesis(genesis) {
  if (genesis !== DEVNET_GENESIS) {
    throw new Error(`Devnet required; refusing another cluster (received genesis: ${genesis})`);
  }
}

export function deriveSkill(programId, authority, hash) {
  return PublicKey.findProgramAddressSync([Buffer.from('skill'), authority.toBuffer(), hash], programId)[0];
}

export async function connect({ local = false, readOnly = false } = {}) {
  const url = process.env.ANCHOR_PROVIDER_URL || DEVNET_URL;
  if (local) {
    if (!['localhost', '127.0.0.1', '[::1]'].includes(new URL(url).hostname)) throw new Error('Chain tests require a local validator');
  }
  const connection = new anchor.web3.Connection(url, 'confirmed');
  if (!local) assertDevnetGenesis(await connection.getGenesisHash());
  const idl = JSON.parse(readFileSync(new URL('../target/idl/jrskill.json', import.meta.url), 'utf8'));
  let provider;
  if (readOnly) {
    provider = { connection };
  } else {
    const path = process.env.ANCHOR_WALLET || resolve(homedir(), '.config/solana/id.json');
    const keypair = anchor.web3.Keypair.fromSecretKey(Uint8Array.from(JSON.parse(readFileSync(path, 'utf8'))));
    provider = new anchor.AnchorProvider(connection, new anchor.Wallet(keypair), { commitment: 'confirmed' });
  }
  const program = new anchor.Program(idl, provider);
  const info = await connection.getAccountInfo(program.programId);
  if (!info?.executable) throw new Error('Program is not deployed at the IDL address');
  return { program, provider, connection };
}

export async function readVerified(program, authority, original = loadPayload()) {
  const pda = deriveSkill(program.programId, authority, original.payloadHash);
  const info = await program.provider.connection.getAccountInfo(pda);
  if (!info || !info.owner.equals(program.programId)) throw new Error('Missing Skill or invalid account owner');
  const account = program.coder.accounts.decode('skill', info.data); // Checks discriminator.
  return { pda, recovered: verifySkill(account, authority, original) };
}

export async function createInstruction(program, authority, original) {
  const pda = deriveSkill(program.programId, authority, original.payloadHash);
  return program.methods.createSkill(original.schemaVersion, [...original.payloadHash], original.payload)
    .accountsStrict({ skill: pda, authority, systemProgram: SystemProgram.programId }).instruction();
}
