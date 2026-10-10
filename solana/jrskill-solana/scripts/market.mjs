import { connect, PublicKey, SystemProgram } from './client.mjs';
import { bn, integer, marketAddress, offerAddress, programDataAddress, terms, readLicense, buyInstruction, LICENSE_BYTES } from './commerce.mjs';
import { Transaction } from './client.mjs';

const [operation, ...args] = process.argv.slice(2);
const allowed = { configure: 2, offer: 2, buy: 3, read: 2 };
if (!(operation in allowed) || args.length !== allowed[operation]) {
  throw new Error('Usage: configure TREASURY FEE_BPS | offer SKILL PRICE_LAMPORTS | buy SKILL MAX_LAMPORTS --quote/--send | read BUYER SKILL');
}
const { program, provider, connection } = await connect({ readOnly: operation === 'read' });
let signature;
if (operation === 'configure') {
  const treasury = new PublicKey(args[0]); const fee = Number(integer(args[1], 10000n));
  if (treasury.equals(PublicKey.default)) throw new Error('Invalid treasury');
  signature = await program.methods.initializeMarket(treasury, fee).accountsStrict({ market: marketAddress(program), admin: provider.wallet.publicKey, program: program.programId, programData: programDataAddress(program), systemProgram: SystemProgram.programId }).rpc();
  console.log(JSON.stringify({ cluster: 'devnet', market: marketAddress(program).toBase58(), treasury: treasury.toBase58(), fee_bps: fee, signature }));
} else if (operation === 'offer') {
  const skill = new PublicKey(args[0]); const price = bn(args[1]);
  if (price.isZero()) throw new Error('Price must be positive');
  signature = await program.methods.createOffer(price).accountsStrict({ market: marketAddress(program), skill, offer: offerAddress(program, skill), creator: provider.wallet.publicKey, systemProgram: SystemProgram.programId }).rpc();
  console.log(JSON.stringify({ cluster: 'devnet', offer: offerAddress(program, skill).toBase58(), skill: skill.toBase58(), price_lamports: price.toString(), signature }));
} else if (operation === 'buy') {
  const skill = new PublicKey(args[0]); const maximum = integer(args[1]);
  if (!['--quote', '--send'].includes(args[2])) throw new Error('Choose --quote or --send explicitly');
  const buyer = provider.wallet.publicKey;
  const existing = await readLicense(program, buyer, skill);
  if (existing.license) {
    console.log(JSON.stringify({ cluster: 'devnet', buyer: buyer.toBase58(), license: existing.address.toBase58(), already_exists: true }));
  } else {
    const { market, offer, shares } = await terms(program, skill);
    if (maximum < BigInt(offer.priceLamports.toString())) throw new Error('Price exceeds accepted maximum');
    if (buyer.equals(offer.creator) || buyer.equals(market.treasury)) throw new Error('Use a buyer different from payment recipients');
    const transaction = new Transaction().add(await buyInstruction(program, buyer, skill, maximum));
    transaction.feePayer = buyer;
    transaction.recentBlockhash = (await connection.getLatestBlockhash()).blockhash;
    const fee = await connection.getFeeForMessage(transaction.compileMessage());
    if (fee.value === null) throw new Error('Unable to estimate transaction fee');
    const rent = await connection.getMinimumBalanceForRentExemption(LICENSE_BYTES);
    console.log(JSON.stringify({ cluster: 'devnet', buyer: buyer.toBase58(), skill: skill.toBase58(), price_lamports: offer.priceLamports.toString(), creator: offer.creator.toBase58(), creator_lamports: shares.creator.toString(), treasury: market.treasury.toBase58(), treasury_lamports: shares.treasury.toString(), rent_lamports: rent, estimated_fee_lamports: fee.value, transaction_bytes: transaction.serialize({ requireAllSignatures: false }).length, sends_transaction: args[2] === '--send' }));
    if (args[2] === '--send') {
      signature = await provider.sendAndConfirm(transaction);
      const result = await readLicense(program, buyer, skill);
      if (!result.license) throw new Error('Confirmed transaction but license not found');
      console.log(JSON.stringify({ cluster: 'devnet', signature, license: result.address.toBase58(), verified: true }));
    }
  }
} else {
  const buyer = new PublicKey(args[0]); const skill = new PublicKey(args[1]);
  const { address, license } = await readLicense(program, buyer, skill);
  console.log(JSON.stringify({ cluster: 'devnet', license: address.toBase58(), exists: !!license, buyer: buyer.toBase58(), skill: skill.toBase58(), model_version: license?.modelVersion, price_paid: license?.pricePaid.toString(), creator_paid: license?.creatorPaid.toString(), treasury_paid: license?.treasuryPaid.toString() }));
}
