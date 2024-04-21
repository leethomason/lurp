#include "battle.h"
#include "scriptbridge.h"
#include "test.h"
#include "scriptasset.h"
#include "varbinder.h"

#include <algorithm>
#include <assert.h>
#include <fmt/core.h>
#include <vector>
#include <numeric>

namespace lurp {
namespace swbattle {

BattleSystem::BattleSystem(const ScriptAssets& assets, const VarBinder& binder, const lurp::Battle& battle, EntityID playerID, Random& r)
	: _random(r)
{
	setBattlefield(battle.name);
	for (const Region& region : battle.regions) {
		addRegion(region);
	}
	const Actor& actor = assets.getActor(playerID);
	SWCombatant swPlayer = SWCombatant::convert(actor, assets, binder);
	addCombatant(swPlayer);

	for (const EntityID& comID : battle.combatants) {
		const Combatant& c = assets.getCombatant(comID);
		for (int i = 0; i < c.count; i++) {
			SWCombatant swC = SWCombatant::convert(c, assets);
			addCombatant(swC);
		}
	}
}

Roll BattleSystem::doRoll(Random& random, Die d, bool useWild)
{
	Roll r;
	r.die = d;

	int nAce = 0;
	r.total[0] = d.roll(random, &nAce);
	TEST(r.nAce(0) == nAce);

	if (useWild) {
		Die wild = Die(1, 6, d.b);
		r.total[1] = wild.roll(random, &nAce);
		TEST(r.nAce(1) == nAce);
	}

	return r;
}

std::string ModTypeName(ModType type)
{
	switch (type) {
	case ModType::kRangeBuff: return "RangeBuff";
	case ModType::kMeleeBuff: return "MeleeBuff";
	case ModType::kCombatBuff: return "CombatBuff";

	case ModType::kDefend: return "Defend";
	case ModType::kGangUp: return "GangUp";
	case ModType::kUnarmed: return "Unarmed";
	case ModType::kRange: return "Range";
	case ModType::kCover: return "Cover";

	case ModType::kBolt: return "Bolt";
	case ModType::kHeal: return "Heal";
	}
	assert(false);
	return "";
}

ModType ModTypeFromName(const std::string& _name)
{
	std::string name = toLower(_name);

	// Only things that can be specified as powers in
	// the game description are valid values here.
	if (name == "randebuff") return ModType::kRangeBuff;
	if (name == "meleebuff") return ModType::kMeleeBuff;
	if (name == "combatbuff") return ModType::kCombatBuff;
	if (name == "bolt") return ModType::kBolt;
	if (name == "heal") return ModType::kHeal;
	PLOG(plog::error) << "Unknown ModType: " << _name;
	assert(false);
	return ModType::kBolt;
}



void BattleSystem::setBattlefield(const std::string& name)
{
	_name = name;
	_regions.clear();
}

void BattleSystem::addRegion(const Region& r)
{
	_regions.push_back(r);
	std::sort(_regions.begin(), _regions.end(), [](const Region& a, const Region& b) { return a.yards < b.yards; });
}

void BattleSystem::addCombatant(SWCombatant add)
{
	add.team = _combatants.empty() ? 0 : 1;
	add.index = (int)_combatants.size();
	if (add.region < 0) {
		if (add.team == 0)
			add.region = 0;
		else
			add.region = (int)_regions.size() - 1;
	}
	_combatants.push_back(add);
}

std::pair<Die, int> BattleSystem::calcMelee(int attackerIndex, int defenderIndex, std::vector<ModInfo>& mods) const
{
	const SWCombatant& attacker = _combatants[attackerIndex];
	const SWCombatant& defender = _combatants[defenderIndex];
	Die atkDie = attacker.fighting;

	int nAttackers = (int)combatants(attacker.team, attacker.region, true, -1).size();
	int nDefenders = (int)combatants(defender.team, defender.region, true, -1).size();
	int gangUp = std::max(0, nAttackers - nDefenders);
	if (gangUp) {
		mods.push_back({ attacker.index, ModType::kGangUp, gangUp, nullptr });
		atkDie.b += gangUp;
	}

	bool isUnarmed = (!attacker.unArmed() && defender.unArmed());
	if (isUnarmed) {
		mods.push_back({ attacker.index, ModType::kUnarmed, kUnarmedTN, nullptr });
		atkDie.b += kUnarmedTN;
	}
	atkDie.b += applyMods(
		uint32_t(ModType::kMeleeBuff) | uint32_t(ModType::kCombatBuff),
		attacker.activePowers, 
		mods);

	// FIXME: Should defender boost increase parry?
	return std::make_pair(atkDie, defender.parry());
}

std::pair<Die, int> BattleSystem::calcRanged(int attackerIndex, int defenderIndex, std::vector<ModInfo>& mods) const
{
	const SWCombatant& attacker = _combatants[attackerIndex];
	const SWCombatant& defender = _combatants[defenderIndex];
	if (!attacker.hasRanged())
		return std::make_pair(Die(0, 0, 0), 0);

	Die atkDie = attacker.shooting;
	atkDie.b += applyMods(
		uint32_t(ModType::kRangeBuff) | uint32_t(ModType::kCombatBuff),
		attacker.activePowers, 
		mods);

	// Should range and cover be applied to the roll or the TN?
	// I think the roll, because of the negative numbers.
	int yards = distance(attacker.region, defender.region);
	int rangeB = 0;
	int shortRange = attacker.rangedWeapon.range / 2;
	int medRange = attacker.rangedWeapon.range;
	int longRange = attacker.rangedWeapon.range * 2;
	if (yards > shortRange && yards <= medRange)
		rangeB = -2;
	else if (yards > medRange && yards <= longRange)
		rangeB = -4;
	else if (yards > longRange) {
		rangeB = -6;	// extreme range is only possible with aim, which isn't implemented yet, and likely
		// not useful given the size of the battlefield. So even though this is wrong, it's not a big deal.
	}
	if (rangeB) {
		atkDie.b += rangeB;
		mods.push_back({ attacker.index, ModType::kRange, rangeB, nullptr });
	}

	int coverB = -int(_regions[defender.region].cover) * 2;
	if (coverB) {
		atkDie.b += coverB;
		mods.push_back({ attacker.index, ModType::kCover, coverB, nullptr });
	}
	//atkDie.b += applyMods(ModType::kPowerCover, defender.activePowers, mods, -1);
	return std::make_pair(atkDie, 4);
}

// Remember that the power is a MULTIPLIER, so the actual mod is double the multiplier.
// Confusing; but a challenge of the SW rules.
int BattleSystem::applyMods(uint32_t filter, const std::vector<ActivePower>& powers, std::vector<ModInfo>& applied)
{
	int result = 0;
	for (const ActivePower& ap : powers) {
		if (uint32_t(ap.power->type) & filter) {
			result += ap.power->deltaDie();
			applied.push_back({ ap.caster, ap.power->type, ap.power->deltaDie(), ap.power});
		}
	}
	return result;
}

double BattleSystem::chance(int tn, Die die)
{
	// Bonus and TN are confusingly intermingled in SW
	int t = tn - die.b;
	// But wait! A natural 1 on the first roll is always a failure.
	if (t <= 2)
		t = 2;

	// take a d4
	// 1	1.00	4/4 of 100
	// 2	0.75	3/4 of 100
	// 3	0.50	2/4 of 100
	// 4+  (0.25)	1/4 of 100
	// 5	0.25	4/4 of 25
	// 6	0.19	3/4 of 25
	// 7	0.12	2/4 of 25
	// 8+	0.06	1/4 of 25
	int nAce = (t - 1) / die.d;
	double chance1 = pow(1.0 / double(die.d), double(nAce));
	int tFinal = t - nAce * die.d;
	double chance2 = 1.0 - (double(tFinal) - 1.0) / double(die.d);
	double chance = chance1 * chance2;
	return chance;
}

double BattleSystem::wildChance(int tn, Die die)
{
	double c = chance(tn, die);
	double w = chance(tn, Die(1, 6, die.b));
	return chanceAorBSucceeds(c, w);
}

void SWCombatant::endTurn()
{
	if (!shaken && !flags[kHasUsedAction]) {
		flags.reset();
		flags.set(kDefend);
	}
	else {
		flags.reset();
	}
}

void BattleSystem::start(bool shuffleTurnOrder)
{
	_turnOrder.resize(_combatants.size());
	for (size_t i = 0; i < _turnOrder.size(); i++)
		_turnOrder[i] = int(i);
	if (shuffleTurnOrder)
		_random.shuffle(_turnOrder.begin(), _turnOrder.end());
}

bool BattleSystem::victory() const
{
	for (size_t i = 1; i < _combatants.size(); ++i)
		if (!_combatants[i].dead())
			return false;
	return true;
}

bool BattleSystem::defeat() const
{
	return _combatants[0].dead();
}

bool BattleSystem::done() const 
{
	return victory() || defeat();
}

void BattleSystem::filterActivePowers()
{
	for (SWCombatant& c : _combatants) {
		size_t i = 0;
		while(i < c.activePowers.size()) {
			const ActivePower& ap = c.activePowers[i];
			const SWCombatant& caster = _combatants[ap.caster];
			if (caster.dead()) {
				c.activePowers.erase(c.activePowers.begin() + i);
			}
			else {
				++i;
			}
		}
	}
}

double BattleSystem::powerChance(int caster, const SWPower& power) const
{
	int tn = powerTN(caster, power);
	return chance(tn, _combatants[caster].arcane);
}

int BattleSystem::powerTN(int caster, const SWPower& power) const
{
	return power.tn() + countMaintainedPowers(caster);
}

int BattleSystem::countMaintainedPowers(int index) const
{
	int n = 0;
	for (const SWCombatant& c : _combatants) {
		for (const ActivePower& ap : c.activePowers) {
			if (ap.caster == index)
				n++;
		}
	}
	return n;
}

void BattleSystem::advance() {
	_combatants[turn()].endTurn();
	_orderIndex = (_orderIndex + 1) % _turnOrder.size();

	_combatants[turn()].startTurn();
	recover(turn());
	filterActivePowers();

	// NOTE: SW randomizes the turn order every round. That works well on the
	// tabletop, but without UI support it's confusing in a computer game.
	//if (_orderIndex == 0) {
	//	shuffleOrder();
	//	return true;
	//}
}

std::vector<SWCombatant> BattleSystem::combatants(int team, int region, bool alive, int exclude) const
{
	std::vector<SWCombatant> r;
	int idx = 0;
	for (const SWCombatant& c : _combatants) {
		if (idx++ == exclude)
			continue;
		if ((team < 0 || c.team == team) && (region < 0 || c.region == region)) {
			if (!alive || !c.dead())
				r.push_back(c);
		}
	}
	return r;
}

bool SWCombatant::buffed() const
{
	for(const ActivePower& ap : activePowers)
		if (ap.power->forAllies())
			return true;
	return false;
}

bool SWCombatant::debuffed() const
{
	for (const ActivePower& ap : activePowers)
		if (ap.power->forEnemies())
			return true;
	return false;
}

double SWCombatant::baseMelee() const 
{
	return BattleSystem::chance(4, fighting, wild);
}

double SWCombatant::baseRanged() const
{
	return BattleSystem::chance(4, shooting, wild);
}

Die SWCombatant::convertFromSkill(int skill)
{
	if (skill < 4) return Die(1, 4, -2);
	if (skill > 12) return Die(1, 12, skill - 12);
	return Die(1, (skill / 2) * 2, 0);
}

SWPower SWPower::convert(const lurp::Power& p)
{
	SWPower sp(
		ModTypeFromName(p.effect),
		p.name,
		p.cost,
		p.range,
		p.strength);
	return sp;
}

SWCombatant SWCombatant::convert(const lurp::Combatant& c, const ScriptAssets& assets)
{
	SWCombatant r;
	r.link = c.entityID;
	r.name = c.name;
	r.wild = c.wild;
	r.bias = c.bias;

	r.fighting = convertFromSkill(c.fighting);
	r.shooting = convertFromSkill(c.shooting);
	r.arcane = convertFromSkill(c.arcane);

	const Item* mw = c.inventory.meleeWeapon();
	if (mw) {
		r.meleeWeapon.name = mw->name;
		r.meleeWeapon.damageDie = mw->damage;
	}
	const Item* rw = c.inventory.rangedWeapon();
	if (rw) {
		r.rangedWeapon.name = rw->name;
		r.rangedWeapon.damageDie = rw->damage;
		r.rangedWeapon.ap = rw->ap;
		r.rangedWeapon.range = rw->range;
	}
	const Item* armor = c.inventory.armor();
	if (armor) {
		r.armor.name = armor->name;
		r.armor.armor = armor->armor;
	}
	for (const EntityID& pid : c.powers) {
		const Power& p = assets.getPower(pid);
		SWPower sp = SWPower::convert(p);
		r.powers.push_back(sp);
	}
	return r;
}

SWCombatant SWCombatant::convert(const lurp::Actor& c, const ScriptAssets& assets, const VarBinder& bind)
{
	SWCombatant r;
	r.link = c.entityID;
	r.name = c.name;
	r.wild = c.wild;

	int fighting = (int) bind.get(c.entityID + "." + "fighting").num;
	int shooting = (int) bind.get(c.entityID + "." + "shooting").num;
	int arcane = (int) bind.get(c.entityID + "." + "arcane").num;

	r.fighting = convertFromSkill(fighting);
	r.shooting = convertFromSkill(shooting);
	r.arcane = convertFromSkill(arcane);

	const Inventory& inv = assets.getInventory(c);

	const Item* mw = inv.meleeWeapon();
	if (mw) {
		r.meleeWeapon.name = mw->name;
		r.meleeWeapon.damageDie = mw->damage;
	}
	const Item* rw = inv.rangedWeapon();
	if (rw) {
		r.rangedWeapon.name = rw->name;
		r.rangedWeapon.damageDie = rw->damage;
		r.rangedWeapon.ap = rw->ap;
		r.rangedWeapon.range = rw->range;
	}
	const Item* armor = inv.armor();
	if (armor) {
		r.armor.name = armor->name;
		r.armor.armor = armor->armor;
	}
	for (const EntityID& pid : c.powers) {
		const Power& p = assets.getPower(pid);
		SWPower sp = SWPower::convert(p);
		r.powers.push_back(sp);
	}
	return r;
}

BattleSystem::ActionResult BattleSystem::checkAttack(int combatant, int target) const
{
	const SWCombatant& src = _combatants[combatant];
	const SWCombatant& dst = _combatants[target];

	assert(combatant != target);
	if (!src.canAttack())
		return ActionResult::kNoAction;
	if (src.team == dst.team)
		return ActionResult::kInvalid;
	if (src.region != dst.region && !src.hasRanged())
		return ActionResult::kInvalid;
	return ActionResult::kSuccess;
}

BattleSystem::ActionResult BattleSystem::checkMove(int combatant, int dir) const
{
	const SWCombatant& src = _combatants[combatant];
	if (src.flags[SWCombatant::kHasMoved])
		return ActionResult::kNoAction;
	if (!within(src.region + dir, 0, (int)_regions.size()))
		return ActionResult::kInvalid;
	return ActionResult::kSuccess;
}

BattleSystem::ActionResult BattleSystem::checkPower(int combatant, int target, int pIndex) const
{
	const SWCombatant& src = _combatants[combatant];
	const SWCombatant& dst = _combatants[target];
	const SWPower* power = &src.powers[pIndex];

	int powerRange = power->rangeMult * src.smarts();
	int targetRange = distance(src.region, dst.region);

	if (powerRange < targetRange)
		return ActionResult::kOutOfRange;
	if (src.team != dst.team && power->forAllies())
		return ActionResult::kInvalid;
	if (src.team == dst.team && power->forEnemies())
		return ActionResult::kInvalid;
	if (power->type == ModType::kHeal && dst.wounds == 0)
		return ActionResult::kInvalid;
	if (dst.dead() || src.dead())
		return ActionResult::kInvalid;
	return ActionResult::kSuccess;
}

void BattleSystem::place(int combataint, int region)
{
	_combatants[combataint].region = region;
}

BattleSystem::ActionResult BattleSystem::move(int id, int dir)
{
	SWCombatant& src = _combatants[id];

	if (src.flags[SWCombatant::kHasMoved])
		return ActionResult::kNoAction;
	if (!within(src.region + dir, 0, (int)_regions.size()))
		return ActionResult::kInvalid;

	// Check for free attack.
	for (size_t i = 0; i < _combatants.size(); i++) {
		if (i == size_t(id)) continue;
		if (src.dead()) continue;
		if (i != 0 && id != 0) continue;	// FIXME: should really have some sort of "team" notion rather than checking index 0

		if (_combatants[i].region == src.region && _combatants[i].canAttack()) {
			attack(int(i), id, true);
		}
	}
	if (!src.canMove()) return ActionResult::kNoAction;

	src.flags[SWCombatant::kHasMoved] = true;

	MoveAction move = { id, src.region, src.region + dir };
	src.region += dir;
	Action action = { Action::Type::kMove, move };
	queue.push(action);
	return ActionResult::kSuccess;
}

void BattleSystem::applyDamage(SWCombatant& defender, int ap, const Die& die, const Die& strDie, DamageReport& report)
{
	report.damageRoll = doRoll(die, false);
	if (strDie.isValid())
		report.strengthRoll = doRoll(strDie, false);

	report.damage = report.damageRoll.value() + report.strengthRoll.value();
	report.ap = ap;

	int defenderArmor = defender.armor.armor;
	int armor = std::max(0, defenderArmor - ap);	// remove ap from armor
	int toughness = defender.toughness() + armor;

	if (report.damage && report.damage >= toughness) {
		int raise = (report.damage - toughness) / 4;
		if (defender.shaken == false) {
			defender.shaken = true;
			report.defenderShaken = true;
			if (raise) {
				defender.wounds += raise;
				report.defenderWounds = raise;
			}
		}
		else {
			defender.wounds += 1 + raise;
			report.defenderWounds = 1 + raise;
		}
	}
	if (defender.dead()) {
		report.defenderDead = true;
		filterActivePowers();
	}
}

BattleSystem::ActionResult BattleSystem::attack(int attacker, int defender, bool freeAttack)
{
	ActionResult result = checkAttack(attacker, defender);
	if (result != ActionResult::kSuccess)
			return result;

	SWCombatant& src = _combatants[attacker];
	SWCombatant& dst = _combatants[defender];

	src.flags[SWCombatant::kHasUsedAction] = true;

	AttackAction attack;
	attack.attacker = attacker;
	attack.defender = defender;
	attack.melee = (src.region == dst.region);
	attack.freeAttack = freeAttack;

	if (attack.melee) {
		// fixme: free attack should affect melee odds
		auto [atkDie, tn] = calcMelee(attacker, defender, attack.mods);
		attack.attackRoll = doRoll(atkDie, src.wild);

		int value = attack.attackRoll.value();
		attack.success = !attack.attackRoll.criticalFailure() && value >= tn;
		if (attack.success) {
			Die strengthDie(1, src.strength(), 0);
			applyDamage(dst, 0, src.meleeWeapon.damageDie, strengthDie, attack.damageReport);
		}
	}
	else {
		auto [atkDie, tn] = calcRanged(attacker, defender, attack.mods);
		attack.attackRoll = doRoll(atkDie, src.wild);

		int value = attack.attackRoll.value();
		attack.success = !attack.attackRoll.criticalFailure() && value >= tn;
		if (attack.success) {
			applyDamage(dst, src.rangedWeapon.ap, src.rangedWeapon.damageDie, Die(0, 0, 0), attack.damageReport);
		}
	}
	queue.push({ Action::Type::kAttack, attack });
	return ActionResult::kSuccess;
}

BattleSystem::ActionResult BattleSystem::power(int srcIndex, int targetIndex, int pIndex)
{
	SWCombatant& src = _combatants[srcIndex];
	SWCombatant& dst = _combatants[targetIndex];
	const SWPower* power = &src.powers[pIndex];

	PowerAction action;
	action.src = srcIndex;
	action.target = targetIndex;
	action.power = power;

	ActionResult result = checkPower(srcIndex, targetIndex, pIndex);
	if (result != ActionResult::kSuccess)
		return result;

	action.tn = power->tn() + countMaintainedPowers(src.index);
	src.flags[SWCombatant::kHasUsedAction] = true;
	Die die = src.arcane;

	if (power->type == ModType::kBolt) {
		// See notes on Bolt below.
		if (src.region != dst.region) {
			int coverBonus = -int(_regions[dst.region].cover) * 2;
			if (coverBonus) {
				action.mods.push_back({ srcIndex, ModType::kCover, coverBonus, nullptr });
				die.b += coverBonus;
			}
		}
	}

	action.roll = doRoll(die, src.wild);
	action.activated = action.roll.value() >= action.tn;
	if (!action.activated) {
		queue.push({ Action::Type::kPower, action });
		return ActionResult::kSuccess;
	}
	action.activated = true;
	action.raise = (action.roll.value() - action.tn) / 4;

	if (power->region) {
		for (SWCombatant& c : _combatants) {
			if (c.region == dst.region && c.team == dst.team) {
				PowerAction altAction = action;
				powerActivated(srcIndex, power, altAction, c);
			}
		}
	}
	else {
		powerActivated(srcIndex, power, action, dst);
	}
	return ActionResult::kSuccess;
}

void BattleSystem::powerActivated(int srcIndex, const SWPower* power, PowerAction& action, SWCombatant& dst)
{
	if (power->type == ModType::kBolt) {
		// Bolt is WEIRD. The initial roll determines activation, but then modifiers are applied,
		// and that determines to hit. There's just one roll. But that all adds complexity to the system,
		// so it's wrapped into "activation" above.
		// Note the 'effectMult` is an effect multiplier, not an absolute value

		Die d(2, 6, 0);
		d.d += (power->effectMult - 1) * 2; // effect mult increases the die, not number (?)
		d.n += action.raise;

		applyDamage(dst, 0, d, Die(0, 0, 0), action.damageReport);
	}
	else if (power->type == ModType::kHeal) {
		dst.wounds = std::max(0, dst.wounds - power->effectMult - action.raise);
	}
	else {
		ActivePower ap = { srcIndex, power };
		dst.activePowers.push_back(ap);
	}
	queue.push({ Action::Type::kPower, action });
}

std::pair<int, int> BattleSystem::findPower(int combatant) const
{
	const SWCombatant& src = _combatants[combatant];

	std::vector<int> powers(src.powers.size());
	std::iota(powers.begin(), powers.end(), 0);
	_random.shuffle(powers.begin(), powers.end());

	std::vector<int> targets(_combatants.size());
	std::iota(targets.begin(), targets.end(), 0);
	_random.shuffle(targets.begin(), targets.end());

	for (size_t i = 0; i < powers.size(); i++) {
		for (size_t j = 0; j < targets.size(); j++) {
			if (checkPower(combatant, targets[j], powers[i]) == ActionResult::kSuccess) {
				return std::make_pair(powers[i], targets[j]);
			}
		}
	}
	return std::make_pair(-1, -1);
}

void BattleSystem::doEnemyActions()
{
	enum class Type { kAttack, kPower };
	struct Option {
		Type type = Type::kAttack;
		int which = 0;	// which power to use
		int target = 0;
		double score = 0;
	};
	std::vector<Option> options;

	assert(turn() != 0);

	const SWCombatant& src = _combatants[turn()];
	const SWCombatant& player = _combatants[0];

	bool isMelee = src.region == player.region;
	int powerScore =
		src.arcane.summary()
		- (isMelee ? src.fighting.summary() : src.shooting.summary())
		- countMaintainedPowers(turn()) * 2;
	int usePower = powerScore > 0;
	bool usedPower = false;

	if (powerScore > usePower && src.canPowers()) {
		auto [power, target] = findPower(turn());
		if (power >= 0 && target >= 0) {
			this->power(turn(), target, power);
			usedPower = true;
		}
	}

	if (src.hasRanged()) {
		for (int dir = 1; dir >= -1; dir -= 2) {
			if (within(src.region + dir, 0, (int)_regions.size())
				&& _regions[src.region + dir].cover > _regions[src.region].cover
				&& _combatants[0].region != src.region)
			{
				move(turn(), dir);
			}
		}
	}
	bool doMove = true;
	if (src.hasRanged() && src.shooting.summary() > src.fighting.summary())
		doMove = false;
	if (usedPower)
		doMove = false;

	if (doMove) {
		if (src.region != _combatants[0].region) {
			move(turn(), src.region < _combatants[0].region ? 1 : -1);
		}
	}

	if (src.region == _combatants[0].region || src.hasRanged()) {
		attack(turn(), 0, false);
	}
}

void BattleSystem::recover(int combatant)
{
	SWCombatant& src = _combatants[combatant];
	if (src.dead() || !src.shaken || src.flags[SWCombatant::kHasRecovered])
		return;

	src.flags[SWCombatant::kHasRecovered] = true;
	Die spirit(1, src.spirit(), 0);
	spirit.b -= src.wounds;
	Roll roll = doRoll(spirit, src.wild);
	RecoverAction action;
	action.combatant = combatant;

	if (roll.value() >= 4) {
		src.shaken = false;
		action.shaken = false;
	}
	else {
		action.shaken = true;
	}
	queue.push({ Action::Type::kRecover, action });
}

} // namespace lurp
} // namespace swbattle
