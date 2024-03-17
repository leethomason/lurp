#pragma once

#include "util.h"
#include "defs.h"
#include "die.h"

#include <string>
#include <variant>
#include <bitset>

namespace lurp {
struct Combatant;
struct ScriptAssets;
struct Power;
struct Actor;
struct Battle;
class VarBinder;

namespace swbattle {
struct SWPower;

enum class ModType {
	// The general power system really wasn't working.
	// Instead, a more specific set of powers.

	kRangeDebuff,	// penalize range attacks
	kMeleeDebuff,	// penalize melee attacks
	kCombatDebuff,	// penalize all attacks

	// These are caused by situtations
	kDefend,
	kGangUp,
	kUnarmed,
	kRange,
	kCover,

	// These are transient; they should never show up as a mod
	kBolt,	// attack one target
	kHeal,	// heals
};

std::string ModTypeName(ModType type);
ModType ModTypeFromName(const std::string& name);

struct ModInfo {
	int src = 0;
	ModType type;
	int delta = 0;	// FIXME: what does this do??
	const SWPower* power = nullptr;
};

struct SWPower {
	SWPower() {}
	SWPower(ModType type, const std::string& name, int cost, int rangeMult, int effectMult = 1)
		: type(type), name(name), cost(cost), rangeMult(rangeMult), effectMult(effectMult)
	{
		assert(!name.empty());
	}

	ModType type;
	std::string name;
	int cost = 1;
	int rangeMult = 1;
	int effectMult = 1;
	bool region = false;

	int tn() const { return 4 + (cost + 1) / 2; }
	bool forAllies() const {
		if (type == ModType::kHeal) return true;
		if (type == ModType::kBolt) return false;
		return effectMult > 0;
	}
	bool forEnemies() const { return !forAllies(); }
	bool doesDamage() const { return type == ModType::kBolt; }

	int deltaDie() const { return 2 * effectMult; }

	static SWPower convert(const lurp::Power&);
};

struct ActivePower {
	int caster = 0;
	//int raise = 0;	// fixme: not currently implemented. Plenty of complexity here.
	const SWPower* power = nullptr;
};

enum class Cover {
	kNoCover = 0,
	kLightCover = 1,
	kMediumCover = 2,
	kHeavyCover = 3,
	kFullCover = 4,
};

enum class Range {
	kShort,
	kMedium,
	kLong,
	kExtreme,
};

struct MoveAction {
	int combatant = 0;
	int from = 0;
	int to = 0;
};

struct DamageReport {
	Roll damageRoll;					// always used if attack was successful
	Roll strengthRoll;					// for melee only - adds to the total damage
	int ap = 0;							// armor piercing component of the attack
	int damage = 0;
	int defenderWounds = 0;
	bool defenderShaken = false;
	bool defenderDead = false;
};

struct AttackAction {
	int attacker = 0;
	int defender = 0;					// target index to attack
	bool melee = false;					// if true, melee attack, else ranged

	std::vector<ModInfo> mods;			// mods applied to attack (for reporting)
	Roll attackRoll;

	bool freeAttack = false;
	bool success = false;
	DamageReport damageReport;			// set if there was "success"
};

struct RecoverAction {
	int combatant = 0;
	bool shaken = false;
};

struct PowerAction {
	int src = 0;
	int target = 0;
	const SWPower* power = nullptr;
	bool activated = false;
	int raise = 0;
	DamageReport damage;
};

struct Action {
	enum class Type {
		kMove,
		kAttack,
		kRecover,
		kPower,
	};
	Type type;
	std::variant<
		MoveAction,
		AttackAction,
		RecoverAction,
		PowerAction> data;
};

using ActionQueue = Queue<Action>;

struct MeleeWeapon {
	std::string name;		// "longsword"
	Die damageDie;			// damage = STR + damageDie
};

struct RangedWeapon {
	std::string name;		// "bow"
	Die damageDie;			// damage = damageDie
	int ap = 0;				// armor piercing
	int range = 24;			// medium range, yards
};

struct Armor {
	std::string name;
	int armor = 0;
};

struct SWCombatant {
	EntityID link;
	std::string name;
	int index = 0;
	int team = 0;		// 0 player, 1 monster
	int region = -1;

	bool wild = false;

	Die agility;
	Die smarts;
	Die spirit;
	Die strength;
    Die vigor;

	Die fighting = Die(1, 4, -2);	// agility
	Die shooting = Die(1, 4, -2);	// agility
	Die arcane = Die(1, 4, -2);		// smarts

	MeleeWeapon meleeWeapon;
	RangedWeapon rangedWeapon;
	Armor armor;
	std::vector<SWPower> powers;

	int wounds = 0;
	bool shaken = false;
	bool dead() const { return wild ? wounds > 3 : wounds > 0; }
	std::vector<ActivePower> activePowers;

	int toughness() const { return 2 + vigor.d / 2; }
	int parry() const { return 2 + fighting.d / 2; }

	enum {
		kHasRecovered = 0,
		kHasMoved,
		kHasUsedAction,
		kDefend,
	};
	std::bitset<32> flags;

	void startTurn() { flags.reset(); }
	void endTurn();

	bool canMove() const { return !dead() && !flags[kHasMoved]; }
	bool canAttack() const { return !dead() && !flags[kHasUsedAction] && !shaken; }
	bool canPowers() const { return !dead() && !flags[kHasUsedAction] && !shaken && !powers.empty(); }

	bool hasRanged() const { return !rangedWeapon.name.empty(); }
	bool hasMelee() const { return !meleeWeapon.name.empty(); }
	bool unArmed() const { return meleeWeapon.name.empty(); }
	bool hasArmor() const { return !armor.name.empty(); }

	bool buffed() const;
	bool debuffed() const;

	double baseMelee() const;
	double baseRanged() const;

	// Auto "level" where n is the number of advances (although this system much
	// more narrowly defines advances than Savage Worlds, so that isn't quite correct either.)
	// Very roughly: an advance will increase a die, witht the priorities given. So calling with:
	//    `autoLevel(n, 6, 4, 2)`
	// Will generally move to a fighting die 2 higher than the shooting, and the shooting 2 higher than 
	// the arcane. `n` should be in the range of 1 to ~20. If any of fighting/shooting/arcane are
	// 0, that skill tree will be ignored.
	// The `seed` adds a little wiggle randomness to the auto-level.
	void autoLevel(int n, int fighting, int shooting, int arcane, uint32_t seed = 0);

	bool autoLevelFighting();
	bool autoLevelShooting();
	bool autoLevelArcane();

	static SWCombatant convert(const lurp::Combatant& c, const ScriptAssets&);
	static SWCombatant convert(const lurp::Actor& c, const ScriptAssets&, const VarBinder& bind);
	static Die convertFromSkill(int skill);
};

struct Region {
	std::string name;
	int yards = 0;
	Cover cover = Cover::kNoCover;
};

class BattleSystem {
public:
	static constexpr int kDefendTN = 4;
	static constexpr int kUnarmedTN = 2;

	// --------- Initialization from Script --------
	BattleSystem(const ScriptAssets& assets, const VarBinder& binder, const lurp::Battle& battle, EntityID player, Random& r);

	// --------- Initialization --------
	BattleSystem(Random& r) : _random(r) {}
	void setBattlefield(const std::string& name);
	void addRegion(const Region& region);
	// Player must be first!
	void addCombatant(SWCombatant c);
	void start(bool randomTurnOrder = true);

	// --------- Status --------
	const std::string& name() const { return _name; }
	const std::vector<Region>& regions() const { return _regions; }
	const std::vector<SWCombatant>& combatants() const { return _combatants; }
	int distance(int region0, int region1) const {
		return std::abs(_regions[region0].yards - _regions[region1].yards);
	}
	std::vector<SWCombatant> combatants(int team, int region, bool alive, int exclude) const; // -1 for any team or region

	ActionQueue queue;

	// --------- Turn Order --------
	int turn() const { return _turnOrder[_orderIndex]; }
	bool playerTurn() const { return turn() == 0; }
	const std::vector<int>& turnOrder() const { return _turnOrder; }

	enum class ActionResult {
		kSuccess,
		kNoAction,
		kOutOfRange,
		kInvalid,
	};

	ActionResult attack(int combatant, int target, bool freeAttack=false);
	ActionResult move(int combatant, int dir);
	ActionResult power(int combatant, int target, int power);

	ActionResult checkAttack(int combatant, int target) const;
	ActionResult checkMove(int combatant, int dir) const;
	ActionResult checkPower(int combatant, int target, int power) const;

	// Just moves the combatant to the new region, no checks. Used for
	// testing and setting up the battle.
	void place(int combataint, int region);

	void doEnemyActions();
	void advance();

	bool done() const;
	bool victory() const;	// If battle aborted, then neither victory() or defeat().
	bool defeat() const;	// With normal battle run, one of victory() or defeat() is set

	// --------- Utility --------
	std::pair<Die, int> calcMelee(int attacker, int target, std::vector<ModInfo>& mods) const;
	std::pair<Die, int> calcRanged(int attacker, int target, std::vector<ModInfo>& mods) const;
	
	int powerTN(int caster, const SWPower& power) const;
	double powerChance(int caster, const SWPower& power) const;

	Roll doRoll(Die d, bool useWild) { return doRoll(_random, d, useWild); }
	static Roll doRoll(Random& random, Die d, bool useWild);

	// --------- Chance Computation --------
	// Note that the "ace" system causes some odd appearing statistical "steps". For example, consider
	// a d6. There's no chance of rolling a 6 (since it aces.) The possible values go 5 then 7. a TN of 6 
	// or 7 has the same likelihood.
	static double chance(int tn, Die die);
	static double wildChance(int tn, Die die);
	static double chance(int tn, Die die, bool useWild) {
		if (useWild) return wildChance(tn, die);
		return chance(tn, die);
	}

	static double chanceAorBSucceeds(double a, double b) {
		return 1.0 - (1.0 - a) * (1.0 - b);
	}

	// internal
	static int applyMods(ModType type, const std::vector<ActivePower>& potential, std::vector<ModInfo>& applied);

private:
	int countMaintainedPowers(int combatant) const;
	void recover(int combatant);
	void filterActivePowers();
	void powerActivated(int srcIndex, const SWPower* power, PowerAction& action, SWCombatant& dst);

	// power, target
	std::pair<int, int> findPower(int combatant) const;

	void applyDamage(SWCombatant& defender, int ap, const Die& die, const Die& strDie, DamageReport& report);

	Random& _random;
	int _orderIndex = 0;
	std::string _name;
	std::vector<SWCombatant> _combatants;
	std::vector<Region> _regions;
	std::vector<int> _turnOrder;
	std::vector<ActivePower> _activePowers;
};

} // namespace lurp
} // namespace swbattle
