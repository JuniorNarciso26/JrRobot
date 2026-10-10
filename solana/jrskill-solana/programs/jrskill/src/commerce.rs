use crate::{Skill, SkillError};
use anchor_lang::{
    prelude::*,
    system_program::{transfer, Transfer},
};

pub fn initialize(ctx: Context<InitializeMarket>, treasury: Pubkey, fee_bps: u16) -> Result<()> {
    require!(fee_bps <= 10_000, SkillError::InvalidFee);
    require!(treasury != Pubkey::default(), SkillError::InvalidTreasury);
    let market = &mut ctx.accounts.market;
    market.admin = ctx.accounts.admin.key();
    market.treasury = treasury;
    market.fee_bps = fee_bps;
    Ok(())
}

pub fn offer(ctx: Context<CreateOffer>, price_lamports: u64) -> Result<()> {
    require!(price_lamports > 0, SkillError::InvalidPrice);
    let offer = &mut ctx.accounts.offer;
    offer.skill = ctx.accounts.skill.key();
    offer.creator = ctx.accounts.creator.key();
    offer.price_lamports = price_lamports;
    Ok(())
}

pub fn buy(ctx: Context<BuyLicense>, max_price_lamports: u64) -> Result<()> {
    let price = ctx.accounts.offer.price_lamports;
    require!(price <= max_price_lamports, SkillError::PriceLimit);
    let buyer = ctx.accounts.buyer.key();
    require!(
        buyer != ctx.accounts.creator.key() && buyer != ctx.accounts.treasury.key(),
        SkillError::BuyerIsRecipient
    );
    // u64 * u16 fits u128. Floor the fee and give the remainder to the creator.
    let fee = ((price as u128 * ctx.accounts.market.fee_bps as u128) / 10_000) as u64;
    let creator_amount = price - fee;
    for (recipient, amount) in [
        (ctx.accounts.creator.to_account_info(), creator_amount),
        (ctx.accounts.treasury.to_account_info(), fee),
    ] {
        if amount > 0 {
            transfer(
                CpiContext::new(
                    ctx.accounts.system_program.key(),
                    Transfer {
                        from: ctx.accounts.buyer.to_account_info(),
                        to: recipient,
                    },
                ),
                amount,
            )?;
        }
    }
    let license = &mut ctx.accounts.license;
    license.model_version = 0;
    license.buyer = buyer;
    license.skill = ctx.accounts.skill.key();
    license.offer = ctx.accounts.offer.key();
    license.price_paid = price;
    license.creator_paid = creator_amount;
    license.treasury_paid = fee;
    license.issued_at = Clock::get()?.unix_timestamp;
    Ok(())
}

#[derive(Accounts)]
pub struct InitializeMarket<'info> {
    #[account(init, payer = admin, space = MarketConfig::SPACE, seeds = [b"market".as_ref(), b"00".as_ref()], bump)]
    pub market: Account<'info, MarketConfig>,
    #[account(mut)]
    pub admin: Signer<'info>,
    #[account(address = crate::ID, constraint = program.programdata_address()? == Some(program_data.key()) @ SkillError::UnauthorizedAdmin)]
    pub program: Program<'info, crate::program::Jrskill>,
    #[account(constraint = program_data.upgrade_authority_address == Some(admin.key()) @ SkillError::UnauthorizedAdmin)]
    pub program_data: Account<'info, ProgramData>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
pub struct CreateOffer<'info> {
    #[account(seeds = [b"market".as_ref(), b"00".as_ref()], bump)]
    pub market: Account<'info, MarketConfig>,
    #[account(constraint = skill.authority == creator.key() @ SkillError::UnauthorizedCreator)]
    pub skill: Account<'info, Skill>,
    #[account(init, payer = creator, space = Offer::SPACE, seeds = [b"offer".as_ref(), b"00".as_ref(), skill.key().as_ref()], bump)]
    pub offer: Account<'info, Offer>,
    #[account(mut)]
    pub creator: Signer<'info>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
pub struct BuyLicense<'info> {
    #[account(seeds = [b"market".as_ref(), b"00".as_ref()], bump)]
    pub market: Account<'info, MarketConfig>,
    pub skill: Account<'info, Skill>,
    #[account(seeds = [b"offer".as_ref(), b"00".as_ref(), skill.key().as_ref()], bump, has_one = skill, has_one = creator)]
    pub offer: Account<'info, Offer>,
    #[account(init, payer = buyer, space = License::SPACE, seeds = [b"license", buyer.key().as_ref(), skill.key().as_ref()], bump)]
    pub license: Account<'info, License>,
    #[account(mut)]
    pub buyer: Signer<'info>,
    #[account(mut, address = skill.authority)]
    pub creator: SystemAccount<'info>,
    #[account(mut, address = market.treasury)]
    pub treasury: SystemAccount<'info>,
    pub system_program: Program<'info, System>,
}

#[account]
pub struct MarketConfig {
    pub admin: Pubkey,
    pub treasury: Pubkey,
    pub fee_bps: u16,
}
impl MarketConfig {
    pub const SPACE: usize = 8 + 32 + 32 + 2;
}
#[account]
pub struct Offer {
    pub skill: Pubkey,
    pub creator: Pubkey,
    pub price_lamports: u64,
}
impl Offer {
    pub const SPACE: usize = 8 + 32 + 32 + 8;
}
#[account]
pub struct License {
    pub model_version: u8,
    pub buyer: Pubkey,
    pub skill: Pubkey,
    pub offer: Pubkey,
    pub price_paid: u64,
    pub creator_paid: u64,
    pub treasury_paid: u64,
    pub issued_at: i64,
}
impl License {
    pub const SPACE: usize = 8 + 1 + 32 * 3 + 8 * 4;
}
