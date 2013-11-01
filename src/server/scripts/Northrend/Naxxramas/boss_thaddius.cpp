/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"

#include "naxxramas.h"

enum ScriptTexts
{
    SAY_AGGRO               = 0,
    SAY_KILL                = 1,
    SAY_DEATH               = 2,
};

//Stalagg
enum StalagSpells
{
    SPELL_POWERSURGE        = 28134,
    H_SPELL_POWERSURGE      = 54529,
    SPELL_MAGNETIC_PULL     = 28338,
    SPELL_STALAGG_TESLA     = 28097
};

//Feugen
enum FeugenSpells
{
    SPELL_STATICFIELD       = 28135,
    H_SPELL_STATICFIELD     = 54528,
    SPELL_FEUGEN_TESLA      = 28109
};

// Thaddius DoAction
enum ThaddiusActions
{
    ACTION_FEUGEN_RESET,
    ACTION_FEUGEN_DIED,
    ACTION_STALAGG_RESET,
    ACTION_STALAGG_DIED
};

//generic
#define C_TESLA_COIL            16218           //the coils (emotes "Tesla Coil overloads!")
#define EMOTE_TESLA "Tesla Coil overloads!"

//Thaddius
enum ThaddiusScriptTexts
{
    SAY_GREET               = 0, 
    SAY_TAGGRO              = 1,
    SAY_TDEATH              = 2,
    SAY_ELECT               = 3,
    SAY_TKILL               = 4,
    EMOTE_SHIFT             = 5,

    SAY_SCREAM1             = -1533036, //not used
    SAY_SCREAM2             = -1533037, //not used
    SAY_SCREAM3             = -1533038, //not used
    SAY_SCREAM4             = -1533039 //not used
};


enum ThaddiusSpells
{
    SPELL_POLARITY_SHIFT        = 28089,
    SPELL_BALL_LIGHTNING        = 28299,
    SPELL_CHAIN_LIGHTNING       = 28167,
    H_SPELL_CHAIN_LIGHTNING     = 54531,
    SPELL_BERSERK               = 27680,
    SPELL_POSITIVE_CHARGE       = 28062,
    SPELL_POSITIVE_CHARGE_STACK = 29659,
    SPELL_NEGATIVE_CHARGE       = 28085,
    SPELL_NEGATIVE_CHARGE_STACK = 29660,
    SPELL_POSITIVE_POLARITY     = 28059,
    SPELL_NEGATIVE_POLARITY     = 28084,
	SPELL_POSITIVE_POLARITY_H	= 39088,
	SPELL_NEGATIVE_POLARITY_H	= 39091,
};

enum Events
{
    EVENT_NONE,
    EVENT_SHIFT,
    EVENT_CHAIN,
    EVENT_BERSERK,
};

enum Achievement
{
    DATA_POLARITY_SWITCH    = 76047605,
};

// If Feugen or Stalagg gets too far from the Tesla Coil behind him, the raid will start taking unhealable AoE
#define CREATURE_TESLA      16218
#define SPELL_TESLA         28099

#define TESLA_S_X             3450.45f // Stalagg - Tesla
#define TESLA_S_Y            -2931.42f
#define TESLA_S_Z             312.091f
#define TESLA_F_X             3508.14f // Feugen - Tesla
#define TESLA_F_Y            -2988.65f
#define TESLA_F_Z             312.092f


class boss_thaddius : public CreatureScript
{
public:
    boss_thaddius() : CreatureScript("boss_thaddius") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_thaddiusAI (creature);
    }

    struct boss_thaddiusAI : public BossAI
    {
        boss_thaddiusAI(Creature* creature) : BossAI(creature, BOSS_THADDIUS)
        {
            // init is a bit tricky because thaddius shall track the life of both adds, but not if there was a wipe
            // and, in particular, if there was a crash after both adds were killed (should not respawn)

            // Moreover, the adds may not yet be spawn. So just track down the status if mob is spawn
            // and each mob will send its status at reset (meaning that it is alive)
            checkFeugenAlive = false;
            if (Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                checkFeugenAlive = pFeugen->isAlive();

            checkStalaggAlive = false;
            if (Creature* pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                checkStalaggAlive = pStalagg->isAlive();

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_AGGRESSIVE);
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        bool checkStalaggAlive;
        bool checkFeugenAlive;
        bool polaritySwitch;
		bool hasTaunted;
        uint32 uiAddsTimer;

        void Reset()
        {
            _Reset();

            if(Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
            {
                pFeugen->Respawn(true);
                checkFeugenAlive = pFeugen->isAlive();
            }

            if(Creature* pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
            {
                pStalagg->Respawn(true);
                checkStalaggAlive = pStalagg->isAlive();
            }

            if(!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_AGGRESSIVE);
            } else {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_TKILL);
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
            Talk(SAY_TDEATH);
        }

        void DoAction(const int32 action)
        {
            switch (action)
            {
                case ACTION_FEUGEN_RESET:
                    checkFeugenAlive = true;
                    break;
                case ACTION_FEUGEN_DIED:
                    checkFeugenAlive = false;
                    break;
                case ACTION_STALAGG_RESET:
                    checkStalaggAlive = true;
                    break;
                case ACTION_STALAGG_DIED:
                    checkStalaggAlive = false;
                    break;
            }

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                // REACT_AGGRESSIVE only reset when he takes damage.
                DoZoneInCombat();
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            Talk(SAY_TAGGRO);
            events.ScheduleEvent(EVENT_SHIFT, 30000);
            events.ScheduleEvent(EVENT_CHAIN, urand(10000, 20000));
            events.ScheduleEvent(EVENT_BERSERK, 360000);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!hasTaunted && me->IsWithinDistInMap(who, 1000.0f) && who->GetTypeId() == TYPEID_PLAYER)
            {
                Talk(SAY_GREET);
                hasTaunted = true;
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32 & /*uiDamage*/)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void SetData(uint32 id, uint32 data)
        {
            if (id == DATA_POLARITY_SWITCH)
                polaritySwitch = data ? true : false;
        }

        uint32 GetData(uint32 id) const
        {
            if (id != DATA_POLARITY_SWITCH)
                return 0;

            return uint32(polaritySwitch);
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkFeugenAlive && checkStalaggAlive)
                uiAddsTimer = 0;

            if (checkStalaggAlive != checkFeugenAlive)
            {
                uiAddsTimer += diff;
                if (uiAddsTimer > 5000)
                {
                    if (!checkStalaggAlive)
                    {
                        if (instance)
                            if (Creature* pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                                pStalagg->Respawn();
                    }
                    else
                    {
                        if (instance)
                            if (Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                                pFeugen->Respawn();
                    }
                }
            }

            if (!UpdateVictim() && !checkStalaggAlive && !checkStalaggAlive)
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHIFT:
                        DoCastAOE(SPELL_POLARITY_SHIFT);
                        Talk(EMOTE_SHIFT);
                        Talk(SAY_ELECT);
                        events.ScheduleEvent(EVENT_SHIFT, 30000);
                        return;
                    case EVENT_CHAIN:
                        DoCast(me->getVictim(), RAID_MODE(SPELL_CHAIN_LIGHTNING, H_SPELL_CHAIN_LIGHTNING));
                        events.ScheduleEvent(EVENT_CHAIN, urand(10000, 20000));
                        return;
                    case EVENT_BERSERK:
						me->InterruptNonMeleeSpells(false);
                        DoCast(me, SPELL_BERSERK);
                        return;
                }
            }

            if (events.GetTimer() > 15000 && !me->IsWithinMeleeRange(me->getVictim())/* && checkStalaggAlive && checkStalaggAlive*/)
			{
				if(Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 1000, true))
					DoCast(pTarget, SPELL_BALL_LIGHTNING);
			}
        DoMeleeAttackIfReady();
        }
    };

};

class mob_stalagg : public CreatureScript
{
public:
    mob_stalagg() : CreatureScript("mob_stalagg") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_stalaggAI(creature);
    }

    struct mob_stalaggAI : public ScriptedAI
    {
        mob_stalaggAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        uint32 powerSurgeTimer;
        uint32 magneticPullTimer;
		uint32 teslaTimer;

        void Reset()
        {
            if (instance)
                if (Creature* pThaddius = me->GetCreature(*me, instance->GetData64(BOSS_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_STALAGG_RESET);
            powerSurgeTimer = urand(20000, 25000);
            magneticPullTimer = 20000;
			teslaTimer = 5000;
        }

        void EnterCombat(Unit* /*who*/)
        {
			Talk(SAY_AGGRO);
            DoCast(SPELL_STALAGG_TESLA);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                if (Creature* pThaddius = me->GetCreature(*me, instance->GetData64(BOSS_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_STALAGG_DIED);
			Talk(SAY_DEATH);
        }

        void KilledUnit(Unit* /*victim*/)
        {
                Talk(SAY_KILL);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if(magneticPullTimer <= uiDiff)
            {
                if(Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                {
                    Unit* pStalaggVictim = me->getVictim();
                    Unit* pFeugenVictim = pFeugen->getVictim();

                    if(pFeugenVictim && pStalaggVictim)
                    {
                        pFeugen->getThreatManager().addThreat(pStalaggVictim, pFeugen->getThreatManager().getThreat(pFeugenVictim));
                        me->getThreatManager().addThreat(pFeugenVictim, me->getThreatManager().getThreat(pStalaggVictim));
                        pFeugen->getThreatManager().modifyThreatPercent(pFeugenVictim, -100);
                        me->getThreatManager().modifyThreatPercent(pStalaggVictim, -100);

                        pFeugenVictim->JumpTo(me, 0.3f);
                        pStalaggVictim->JumpTo(pFeugen, 0.3f);
                    }
                }

                magneticPullTimer = 20000;
            } else magneticPullTimer -= uiDiff;

            if (powerSurgeTimer <= uiDiff)
            {
                DoCast(me, RAID_MODE(SPELL_POWERSURGE, H_SPELL_POWERSURGE));
                powerSurgeTimer = urand(15000, 20000);
            } else powerSurgeTimer -= uiDiff;

	        if (teslaTimer <= uiDiff)
			{
                Creature* Tesla = me->SummonCreature(CREATURE_TESLA, TESLA_S_X, TESLA_S_Y, TESLA_S_Z, 0, TEMPSUMMON_TIMED_DESPAWN, 2000);
                teslaTimer = 5000;
                    if (Tesla){
                        Tesla->SetVisible(false);
                        if (!me->IsWithinDistInMap(Tesla, 20)){
                            Tesla->MonsterTextEmote(EMOTE_TESLA, 0, true);
							if(Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 1000, true))
								DoCast(pTarget, SPELL_TESLA, true);
							teslaTimer = 500;
                        }
                    }
            }
			else teslaTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

};

class mob_feugen : public CreatureScript
{
public:
    mob_feugen() : CreatureScript("mob_feugen") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_feugenAI(creature);
    }

    struct mob_feugenAI : public ScriptedAI
    {
        mob_feugenAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        uint32 staticFieldTimer;
		uint32 teslaTimer;

        void Reset()
        {
            if (instance)
                if (Creature* pThaddius = me->GetCreature(*me, instance->GetData64(BOSS_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_FEUGEN_RESET);
            staticFieldTimer = 5000;
			teslaTimer = 5000;
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_FEUGEN_TESLA);
			Talk(SAY_AGGRO);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                if (Creature* pThaddius = me->GetCreature(*me, instance->GetData64(BOSS_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_FEUGEN_DIED);
			Talk(SAY_DEATH);
        }

        void KilledUnit(Unit* /*victim*/)
        {
                Talk(SAY_KILL);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (staticFieldTimer <= uiDiff)
            {
                DoCast(me, RAID_MODE(SPELL_STATICFIELD, H_SPELL_STATICFIELD));
                staticFieldTimer = 5000;
            } else staticFieldTimer -= uiDiff;

	        if (teslaTimer <= uiDiff)
			{
                Creature* Tesla = me->SummonCreature(CREATURE_TESLA, TESLA_F_X, TESLA_F_Y, TESLA_F_Z, 0, TEMPSUMMON_TIMED_DESPAWN, 2000);
                teslaTimer = 5000;
                    if (Tesla){
                        Tesla->SetVisible(false);
                        if (!me->IsWithinDistInMap(Tesla, 20)){
                            Tesla->MonsterTextEmote(EMOTE_TESLA, 0, true);
							if(Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 1000, true))
								DoCast(pTarget, SPELL_TESLA, true);
							teslaTimer = 500;
                        }
                    }
            }
			else teslaTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

};

/*class spell_thaddius_pos_neg_charge : public SpellScriptLoader
{
    public:
        spell_thaddius_pos_neg_charge() : SpellScriptLoader("spell_thaddius_pos_neg_charge") { }

        class spell_thaddius_pos_neg_charge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_thaddius_pos_neg_charge_SpellScript);

            bool Validate(SpellInfo const* /*spell)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_POSITIVE_CHARGE))
                    return false;
                if (!sSpellMgr->GetSpellInfo(SPELL_POSITIVE_CHARGE_STACK))
                    return false;
                if (!sSpellMgr->GetSpellInfo(SPELL_NEGATIVE_CHARGE))
                    return false;
                if (!sSpellMgr->GetSpellInfo(SPELL_NEGATIVE_CHARGE_STACK))
                    return false;
                return true;
            }

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void HandleTargets(std::list<WorldObject*>& targets)
            {
                uint8 count = 0;
                for (std::list<WorldObject*>::iterator ihit = targets.begin(); ihit != targets.end(); ++ihit)
                    if ((*ihit)->GetGUID() != GetCaster()->GetGUID())
                        if (Player* target = (*ihit)->ToPlayer())
                            if (target->HasAura(GetTriggeringSpell()->Id))
                                ++count;

                if (count)
                {
                    uint32 spellId = 0;

                    if (GetSpellInfo()->Id == SPELL_POSITIVE_CHARGE)
                        spellId = SPELL_POSITIVE_CHARGE_STACK;
                    else // if (GetSpellInfo()->Id == SPELL_NEGATIVE_CHARGE)
                        spellId = SPELL_NEGATIVE_CHARGE_STACK;

                    GetCaster()->SetAuraStack(spellId, GetCaster(), count);
                }
            }

            void HandleDamage(SpellEffIndex /*effIndex)
            {
                if (!GetTriggeringSpell())
                    return;

                Unit* target = GetHitUnit();
                Unit* caster = GetCaster();

                if (target->HasAura(GetTriggeringSpell()->Id))
                    SetHitDamage(0);
                else
                {
                    if (target->GetTypeId() == TYPEID_PLAYER && caster->IsAIEnabled)
                        caster->ToCreature()->AI()->SetData(DATA_POLARITY_SWITCH, 1);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_thaddius_pos_neg_charge_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_thaddius_pos_neg_charge_SpellScript::HandleTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_thaddius_pos_neg_charge_SpellScript();
        }
};

class spell_thaddius_polarity_shift : public SpellScriptLoader
{
    public:
        spell_thaddius_polarity_shift() : SpellScriptLoader("spell_thaddius_polarity_shift") { }

        class spell_thaddius_polarity_shift_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_thaddius_polarity_shift_SpellScript);

            bool Validate(SpellInfo const* /*spell)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_POSITIVE_POLARITY) || !sSpellMgr->GetSpellInfo(SPELL_NEGATIVE_POLARITY))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex /)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, roll_chance_i(50) ? SPELL_POSITIVE_POLARITY : SPELL_NEGATIVE_POLARITY, true, NULL, NULL, caster->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_thaddius_polarity_shift_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_thaddius_polarity_shift_SpellScript();
        }
};*/

class achievement_polarity_switch : public AchievementCriteriaScript
{
    public:
        achievement_polarity_switch() : AchievementCriteriaScript("achievement_polarity_switch") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            return target && target->GetAI()->GetData(DATA_POLARITY_SWITCH);
        }
};

void AddSC_boss_thaddius()
{
    new boss_thaddius();
    new mob_stalagg();
    new mob_feugen();
    //new spell_thaddius_pos_neg_charge();
    //new spell_thaddius_polarity_shift();
    new achievement_polarity_switch();
}