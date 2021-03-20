/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2021 EQEMu Development Team (http://eqemulator.org)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#pragma once
#include "mob.h"
#include "bot.h"
#include "../common/timer.h"


class BotClassAttack {

	public:
		BotClassAttack(Bot* bot);

		virtual bool DoClassAttack(Mob *target, bool IsRiposte) { return false; };

		virtual void PerformAttack(Mob *target, bool IsReposte);

		bool DoBashAttack(Mob* target, bool isReposte);

		bool DoKickAttack(Mob* target, bool isReposte);

		bool DoTaunt(Mob* target, bool isReposte);

		StatBonuses& getItemBonuses();

		StatBonuses& getAABonuses();

		StatBonuses& getSpellBonuses();

		static void GetMobsInGroupNeedHeals(Bot *bot, std::vector<Mob*>& mobsOut, bool includePets, float threshold);

		static bool HealRatioComparator(Mob* mob1, Mob* mob2);

		static bool GroupRoleComparator(Mob* mob1, Mob* mob2);

		static bool IsHealingClass(Mob* mob);

		static bool IsTankingClass(Mob* mob);

		static bool IsMeleeClass(Mob* mob);

		static bool IsCasterClass(Mob* mob);

		static bool CanBash(Bot* mob);

	protected:
		Bot* bot;
		Timer kick_timer;
		Timer bash_timer;
		Timer taunt_timer;
		
};


class KnightClassAttack : public BotClassAttack {

	public:
		KnightClassAttack(Bot* bot);

		bool DoClassAttack(Mob *target, bool IsRiposte) override;

	private:
		Timer knight_attack_timer;
};

class RogueAttack : public BotClassAttack {

	public:
		RogueAttack(Bot* bot);

		bool DoClassAttack(Mob *target, bool IsRiposte) override;

	private:
		Timer backstab_timer;
};

class EmptyAttack : public BotClassAttack {

public:
		EmptyAttack(Bot* bot);

		void PerformAttack(Mob *target, bool IsReposte) override;
};

class MonkAttack : public BotClassAttack {

	public:
		MonkAttack(Bot* bot);

		bool DoClassAttack(Mob *target, bool IsRiposte) override;

		void PerformWus(Mob *target, bool IsRiposte);

		EQ::skills::SkillType GetHighestMonkKick();

		EQ::skills::SkillType GetHighestMonkPunch();

	private:
		
		Timer tiger_claw;
		Timer monk_punch;

};

class BerserkerAttack : public BotClassAttack {

	public:
		BerserkerAttack(Bot* bot);

		bool DoClassAttack(Mob *target, bool IsRiposte) override;


	private:
		Timer frenzy;
};
