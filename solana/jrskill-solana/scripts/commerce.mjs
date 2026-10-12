import { createRequire } from 'node:module';
import { PublicKey, SystemProgram } from './client.mjs';
const { BN } = createRequire(import.meta.url)('@anchor-lang/core');
export const LICENSE_BYTES = 137;
export function integer(value, maximum = (1n << 64n) - 1n) {
  if (!/^(0|[1-9][0-9]*)$/.test(String(value))) throw new Error('Expected unsigned decimal integer');
  const result = BigInt(value);
  if (result > maximum) throw new Error('Integer exceeds supported range');
  return result;
}
export const bn = value => new BN(integer(value).toString());
export function split(price, feeBps) {
  price = integer(price); feeBps = integer(feeBps, 10000n);
  const treasury = price * feeBps / 10000n;
  return { creator: price - treasury, treasury };
}
function pda(program, ...seeds) { return PublicKey.findProgramAddressSync(seeds, program.programId)[0]; }
export const marketAddress = program => pda(program, Buffer.from('market'), Buffer.from('00'));
export const offerAddress = (program, skill) => pda(program, Buffer.from('offer'), Buffer.from('00'), skill.toBuffer());
export const licenseAddress = (program, buyer, skill) => pda(program, Buffer.from('license'), buyer.toBuffer(), skill.toBuffer());
export function programDataAddress(program) {
  return PublicKey.findProgramAddressSync([program.programId.toBuffer()], new PublicKey('BPFLoaderUpgradeab1e11111111111111111111111'))[0];
}
export async function account(program, name, address) {
  const info = await program.provider.connection.getAccountInfo(address);
  if (!info) return null;
  if (!info.owner.equals(program.programId)) throw new Error('Invalid account owner');
  return program.coder.accounts.decode(name, info.data);
}
export async function terms(program, skill) {
  const market = await account(program, 'marketConfig', marketAddress(program));
  const offer = await account(program, 'offer', offerAddress(program, skill));
  const content = await account(program, 'skill', skill);
  if (!market || !offer || !content) throw new Error('Market, offer or Skill missing; deploy/configure model 00 first');
  if (!offer.skill.equals(skill) || !offer.creator.equals(content.authority)) throw new Error('Offer/Skill mismatch');
  const shares = split(offer.priceLamports.toString(), market.feeBps);
  if (BigInt(offer.priceLamports.toString()) === 0n) throw new Error('Invalid zero price');
  return { market, offer, shares };
}
export async function readLicense(program, buyer, skill) {
  const address = licenseAddress(program, buyer, skill);
  const license = await account(program, 'license', address);
  if (!license) return { address, license: null };
  const { offer, shares } = await terms(program, skill);
  if (license.modelVersion !== 0 || !license.buyer.equals(buyer) || !license.skill.equals(skill)
      || !license.offer.equals(offerAddress(program, skill)) || !license.pricePaid.eq(offer.priceLamports)
      || license.creatorPaid.toString() !== shares.creator.toString() || license.treasuryPaid.toString() !== shares.treasury.toString()) {
    throw new Error('Invalid license identity or recorded payment');
  }
  return { address, license };
}
export async function buyInstruction(program, buyer, skill, maximum) {
  const { market, offer } = await terms(program, skill);
  return program.methods.buyLicense(bn(maximum)).accountsStrict({
    market: marketAddress(program), skill, offer: offerAddress(program, skill),
    license: licenseAddress(program, buyer, skill), buyer,
    creator: offer.creator, treasury: market.treasury, systemProgram: SystemProgram.programId,
  }).instruction();
}
