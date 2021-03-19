#include "bot_class_attacks.h"

KnightClassAttack::KnightClassAttack(Bot* bot)
: BotClassAttack(bot)
{

}

bool KnightClassAttack::DoClassAttack(Mob * target, bool IsRiposte)
{
	bool ka_time = knight_attack_timer.Check(false);
	if (ka_time) {
		switch (bot->GetClass()) {
		case SHADOWKNIGHT: {
			if (!(target == bot)) {
				bot->CastSpell(SPELL_NPC_HARM_TOUCH, target->GetID());
				knight_attack_timer.Start(HarmTouchReuseTime * 1000);
				return true;
			}
			break;
		}
		case PALADIN: {
			int needsHeals = bot->GetNumberNeedingHealedInGroup(20, false);
			if (needsHeals > 0) {
				std::vector<Mob*> healPots;
				GetMobsInGroupNeedHeals(bot, healPots, false, 20.0f);
				if (healPots.size() > 0) {
					bot->CastSpell(SPELL_LAY_ON_HANDS, healPots.front()->GetID());
					knight_attack_timer.Start(LayOnHandsReuseTime * 1000);
					return true;
				}
				else {
					// shouldnt really be able to get here, if needs heal > 0 we should find heal potentials, check in 2 seconds
					knight_attack_timer.Start(2000);
				}
			}
			else {
				// no one needs heals check in 2 seconds
				knight_attack_timer.Start(2000);
			}
			break;
		}
		default: {
			break;
		}
		}
	}
	return false;
}

BotClassAttack::BotClassAttack(Bot* bot)
:bot(bot)
{
	if (!bot->GetSkill(EQ::skills::SkillBash)) {
		bash_timer.Disable();
	}
	if (!bot->GetSkill(EQ::skills::SkillKick)) {
		kick_timer.Disable();
	}
	if (!IsTankingClass(bot)) {
		taunt_timer.Disable();
	}
}

void BotClassAttack::PerformAttack(Mob *target, bool IsReposte)
{
	if (IsTankingClass(bot)) {
		if (DoTaunt(target, IsReposte)) {
			return;
		}
	}
	if (DoClassAttack(target, IsReposte)) {
		return;
	}
	if (DoBashAttack(target, IsReposte)) {
		return;
	}
	if (DoKickAttack(target, IsReposte)) {
		return;
	}
}

bool BotClassAttack::DoBashAttack(Mob* target, bool isReposte)
{
	if (target == bot) {
		return false;
	}
	if (bash_timer.Check(false) && CanBash(bot)) {
		int dmg = bot->GetBaseSkillDamage(static_cast<EQ::skills::SkillType>(EQ::skills::SkillBash), target);
		bot->DoAnim(animTailRake);
		if (bot->GetWeaponDamage(target, bot->GetBotItem(EQ::invslot::slotSecondary)) <= 0 && bot->GetWeaponDamage(target, bot->GetBotItem(EQ::invslot::slotShoulders)) <= 0)
			dmg = DMG_INVULNERABLE;
		float HasteModifier = (bot->GetHaste() * 0.01f);
		int reuse = (BashReuseTime * 1000);
		bot->DoSpecialAttackDamage(target, EQ::skills::SkillBash, dmg, 0, -1, reuse);
		bash_timer.Start(reuse / HasteModifier);
		return true;
	}
	else {
		return false;
	}
}

bool BotClassAttack::DoKickAttack(Mob* target, bool isReposte)
{
	if (target == bot) {
		return false;
	}
	if (bot->GetLevel() >= RuleI(Combat, NPCBashKickLevel)) {
		if (kick_timer.Check(false)) {
			int dmg = bot->GetBaseSkillDamage(static_cast<EQ::skills::SkillType>(EQ::skills::SkillKick), target);
			bot->DoAnim(animKick);
			if (bot->GetWeaponDamage(target, bot->GetBotItem(EQ::invslot::slotFeet)) <= 0)
				dmg = DMG_INVULNERABLE;

			int reuse = (KickReuseTime * 1000);
			float HasteModifier = (bot->GetHaste() * 0.01f);
			bot->DoSpecialAttackDamage(target, EQ::skills::SkillKick, dmg, 0, -1, reuse);
			kick_timer.Start(reuse / HasteModifier);
			return true;
		}
	}
	return false;
}

bool BotClassAttack::DoTaunt(Mob* target, bool isReposte)
{
	if (bot->taunting && target && target->IsNPC() && taunt_timer.Check(false)) {
		if (bot->GetTarget() && bot->GetTarget()->GetHateTop() && bot->GetTarget()->GetHateTop() != bot) {
			bot->BotGroupSay(bot, "Taunting %s", target->GetCleanName());
			bot->Taunt(target->CastToNPC(), false);
			taunt_timer.Start(TauntReuseTime * 1000);
			return true;
		}
	}
	return false;
}

StatBonuses& BotClassAttack::getItemBonuses()
{
	return bot->itembonuses;
}

StatBonuses& BotClassAttack::getAABonuses()
{
	return bot->aabonuses;
}

StatBonuses& BotClassAttack::getSpellBonuses()
{
	return bot->spellbonuses;
}

void BotClassAttack::GetMobsInGroupNeedHeals(Bot *bot, std::vector<Mob*>& mobsOut, bool includePets, float threshold)
{
	if (bot->HasGroup()) {
		Group* g = bot->GetGroup();
		if (g) {
			for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
				if (g->members[i] && !g->members[i]->qglobal) {
					if (g->members[i]->GetHPRatio() <= threshold) {
						mobsOut.push_back(g->members[i]);
					}
					if (includePets) {
						if (g->members[i]->GetPet() && g->members[i]->GetPet()->GetHPRatio() <= threshold) {
							mobsOut.push_back(g->members[i]->GetPet());
						}
					}
				}
			}
		}
	}
	std::sort(mobsOut.begin(), mobsOut.end(), HealRatioComparator);
	std::sort(mobsOut.begin(), mobsOut.end(), GroupRoleComparator);
}

bool BotClassAttack::HealRatioComparator(Mob* mob1, Mob* mob2)
{
	return mob1->GetHPRatio() < mob2->GetHPRatio();
}

bool BotClassAttack::GroupRoleComparator(Mob* mob1, Mob* mob2)
{
	Group* g = mob1->GetGroup();
	if (g) {
		const char* mainTankName = g->GetMainTankName();
		// first see if we're main tank and move us up
		if (strcasecmp(mob1->GetName(), mainTankName)) {
			return true;
		}
		else if (strcasecmp(mob2->GetName(), mainTankName)) {
			return false;
		}
		// second check if either of us is a healing class
		if (IsHealingClass(mob1) && !IsHealingClass(mob2)) {
			return true;
		}
		else if (IsHealingClass(mob2) && !IsHealingClass(mob1)) {
			return false;
		}
		// final keep order for all others;
		else {
			return true;
		}
	}
	else {
	//not grouped so ordering is nonsensical
		return true;
	}
}

bool BotClassAttack::IsHealingClass(Mob* mob)
{
	switch (mob->GetClass()) {
	case CLERIC:
	case CLERICGM:
	case SHAMAN:
	case SHAMANGM:
	case DRUID:
	case DRUIDGM:
		return true;
	default:
		return false;
	}
}

bool BotClassAttack::IsTankingClass(Mob* mob)
{
	switch (mob->GetClass()) {
	case WARRIOR:
	case WARRIORGM:
	case PALADIN:
	case PALADINGM:
	case SHADOWKNIGHT:
	case SHADOWKNIGHTGM:
	case BERSERKER:
	case BERSERKERGM:
		return true;
	default:
		return false;
	}
}

bool BotClassAttack::IsMeleeClass(Mob* mob)
{
	switch (mob->GetClass()) {
	case BEASTLORD:
	case BEASTLORDGM:
	case ROGUE:
	case ROGUEGM:
	case MONK:
	case MONKGM:
	case RANGER:
	case RANGERGM:
	case BARD:
	case BARDGM:
		return true;
	default:
		return false;
	}
}

bool BotClassAttack::IsCasterClass(Mob* mob)
{
	switch (mob->GetClass()) {
	case ENCHANTER:
	case ENCHANTERGM:
	case MAGICIAN:
	case MAGICIANGM:
	case WIZARD:
	case WIZARDGM:
	case NECROMANCER:
	case NECROMANCERGM:
		return true;
	default:
		return false;
	}
}

bool BotClassAttack::CanBash(Bot* mob)
{
	return mob->GetLevel() >= RuleI(Combat, NPCBashKickLevel) && (mob->GetRace() == OGRE || mob->GetRace() == TROLL || mob->GetRace() == BARBARIAN) ||
	(mob->m_inv.GetItem(EQ::invslot::slotSecondary) && mob->m_inv.GetItem(EQ::invslot::slotSecondary)->GetItem()->ItemType == EQ::item::ItemTypeShield) || 
	(mob->m_inv.GetItem(EQ::invslot::slotPrimary) && mob->m_inv.GetItem(EQ::invslot::slotPrimary)->GetItem()->IsType2HWeapon() && mob->GetAA(aa2HandBash) >= 1);
}

RogueAttack::RogueAttack(Bot* bot)
: BotClassAttack(bot)
{

}

bool RogueAttack::DoClassAttack(Mob *target, bool IsRiposte)
{
	if (backstab_timer.Check(false)) {
		int reuse = (BackstabReuseTime * 1000);
		if (IsRiposte)
			reuse = 0;
		float HasteModifier = (bot->GetHaste() * 0.01f);
		bot->TryBackstab(target, reuse);
		backstab_timer.Start(reuse / HasteModifier);
		return true;
	}
	return false;
}


EmptyAttack::EmptyAttack(Bot* bot)
: BotClassAttack(bot)
{

}

void EmptyAttack::PerformAttack(Mob *target, bool IsReposte)
{

}

MonkAttack::MonkAttack(Bot* bot)
: BotClassAttack(bot)
{

}

bool MonkAttack::DoClassAttack(Mob *target, bool IsRiposte)
{
	float HasteModifier = (bot->GetHaste() * 0.01f);
	if (bot->GetLevel() >= 10) {
		if (tiger_claw.Check(false)) {
			int reuse = (bot->MonkSpecialAttack(target, EQ::skills::SkillTigerClaw) - 1);
			tiger_claw.Start((reuse * 1000) / HasteModifier);
			PerformWus(target, IsRiposte);
			return true;
		}
	}
	if (kick_timer.Check(false)) {
		EQ::skills::SkillType skill = GetHighestMonkAttack();
		int reuse = (bot->MonkSpecialAttack(target, skill) - 1);
		PerformWus(target, IsRiposte);
		kick_timer.Start((reuse * 1000) / HasteModifier);
		return true;
	}
	return false;
}

void MonkAttack::PerformWus(Mob *target, bool IsRiposte)
{

	// Live AA - Technique of Master Wu
	int wuchance = getItemBonuses().DoubleSpecialAttack + getSpellBonuses().DoubleSpecialAttack + getAABonuses().DoubleSpecialAttack;

	if (wuchance) {
		const int MonkSPA[5] = {
			EQ::skills::SkillFlyingKick,
			EQ::skills::SkillDragonPunch,
			EQ::skills::SkillEagleStrike,
			EQ::skills::SkillTigerClaw,
			EQ::skills::SkillRoundKick
		};
		int extra = 0;
		// always 1/4 of the double attack chance, 25% at rank 5 (100/4)
		while (wuchance > 0) {
			if (zone->random.Roll(wuchance)) {
				++extra;
			}
			else {
				break;
			}
			wuchance /= 4;
		}

		Mob* bo = bot->GetBotOwner();
		if (bo && bo->IsClient() && bo->CastToClient()->GetBotOption(Client::booMonkWuMessage)) {

			bo->Message(
				GENERIC_EMOTE,
				"The spirit of Master Wu fills %s!  %s gains %d additional attack(s).",
				bot->GetCleanName(),
				bot->GetCleanName(),
				extra
			);
		}

		auto classic = RuleB(Combat, ClassicMasterWu);
		while (extra) {
			bot->MonkSpecialAttack(bot->GetTarget(), (classic ? MonkSPA[zone->random.Int(0, 4)] : EQ::skills::SkillTigerClaw));
			--extra;
		}
	}
}

EQ::skills::SkillType MonkAttack::GetHighestMonkAttack()
{
	EQ::skills::SkillType skill_to_use = EQ::skills::SkillKick;
	int level = bot->GetLevel();
	if (level >= 30) {
		skill_to_use = EQ::skills::SkillFlyingKick;
	}
	else if (level >= 25) {
		skill_to_use = EQ::skills::SkillDragonPunch;
	}
	else if (level >= 20) {
		skill_to_use = EQ::skills::SkillEagleStrike;
	}
	else if (level >= 5) {
		skill_to_use = EQ::skills::SkillRoundKick;
	}
	return skill_to_use;
}

BerserkerAttack::BerserkerAttack(Bot* bot)
: BotClassAttack(bot)
{

}

bool BerserkerAttack::DoClassAttack(Mob *target, bool IsRiposte)
{

	if (frenzy.Check(false)) {
		int AtkRounds = 3;
		bot->DoAnim(anim2HSlashing);
		int dmg = bot->GetBaseSkillDamage(static_cast<EQ::skills::SkillType>(EQ::skills::SkillFrenzy), target);
		int reuse = (FrenzyReuseTime * 1000);
		while (AtkRounds > 0) {
			if (bot->GetTarget() && (AtkRounds == 1 || zone->random.Int(0, 100) < 75)) {
				bot->DoSpecialAttackDamage(bot->GetTarget(), EQ::skills::SkillFrenzy, dmg, 0, dmg, reuse, true);
			}
			AtkRounds--;
		}
		float HasteModifier = (bot->GetHaste() * 0.01f);
		frenzy.Start(reuse / HasteModifier);
		return true;
	}
	return false;

}
