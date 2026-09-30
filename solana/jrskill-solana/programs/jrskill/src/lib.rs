use anchor_lang::prelude::*;
use sha2::{Digest, Sha256};

// Stage 2 program deployed and validated on Devnet. Preserve its existing keypair.
declare_id!("Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454");

pub const MAX_PAYLOAD_BYTES: usize = 512;

#[program]
pub mod jrskill {
    use super::*;

    pub fn create_skill(
        ctx: Context<CreateSkill>,
        schema_version: u8,
        payload_hash: [u8; 32],
        payload: Vec<u8>,
    ) -> Result<()> {
        require!(schema_version == 1, SkillError::UnsupportedSchema);
        require!(!payload.is_empty(), SkillError::EmptyPayload);
        require!(
            payload.len() <= MAX_PAYLOAD_BYTES,
            SkillError::PayloadTooLarge
        );
        let computed_hash: [u8; 32] = Sha256::digest(&payload).into();
        require!(computed_hash == payload_hash, SkillError::HashMismatch);

        let skill = &mut ctx.accounts.skill;
        skill.authority = ctx.accounts.authority.key();
        skill.schema_version = schema_version;
        skill.payload_hash = payload_hash;
        skill.payload = payload;
        Ok(())
    }
}

#[derive(Accounts)]
#[instruction(schema_version: u8, payload_hash: [u8; 32], payload: Vec<u8>)]
pub struct CreateSkill<'info> {
    #[account(
        init,
        payer = authority,
        space = Skill::SPACE,
        seeds = [b"skill", authority.key().as_ref(), payload_hash.as_ref()],
        bump
    )]
    pub skill: Account<'info, Skill>,
    #[account(mut)]
    pub authority: Signer<'info>,
    pub system_program: Program<'info, System>,
}

#[account]
pub struct Skill {
    pub authority: Pubkey,
    pub schema_version: u8,
    pub payload_hash: [u8; 32],
    pub payload: Vec<u8>,
}

impl Skill {
    // discriminator + pubkey + u8 + hash + Vec length + reserved payload
    pub const SPACE: usize = 8 + 32 + 1 + 32 + 4 + MAX_PAYLOAD_BYTES;
}

#[error_code]
pub enum SkillError {
    #[msg("Only JrSkill schema version 1 is supported")]
    UnsupportedSchema,
    #[msg("Payload must not be empty")]
    EmptyPayload,
    #[msg("Payload exceeds the Stage 2 limit of 512 bytes")]
    PayloadTooLarge,
    #[msg("SHA-256 does not match the original payload bytes")]
    HashMismatch,
}
