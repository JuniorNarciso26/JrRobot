import test from 'node:test';
import assert from 'node:assert/strict';
import { integer, split } from '../scripts/commerce.mjs';
test('integer lamports: fee floor, boundaries and no u64 overflow', () => {
  assert.deepEqual(split(1000000001n, 5000), { creator: 500000001n, treasury: 500000000n });
  const max = (1n << 64n) - 1n;
  assert.deepEqual(split(max, 10000), { creator: 0n, treasury: max });
  assert.deepEqual(split(max, 0), { creator: max, treasury: 0n });
  for (const value of ['-1', '1.5', '1e9', '01', String(1n << 64n)]) assert.throws(() => integer(value));
  assert.throws(() => split(1, 10001));
});
