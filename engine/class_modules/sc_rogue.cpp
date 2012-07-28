// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Custom Combo Point Impl.
// ==========================================================================

struct rogue_t;

#define SC_ROGUE 1

#if SC_ROGUE == 1

#define COMBO_POINTS_MAX 5

static inline int clamp( int x, int low, int high )
{
  return x < low ? low : ( x > high ? high : x );
}

struct combo_points_t
{
  sim_t* sim;
  player_t* player;

  proc_t* proc;
  proc_t* wasted;

  int count;

  combo_points_t( player_t* p ) :
    sim( p -> sim ), player( p ), proc( 0 ), wasted( 0 ), count( 0 )
  {
    proc   = player -> get_proc( "combo_points" );
    wasted = player -> get_proc( "combo_points_wasted" );
  }

  void add( int num, const char* action = 0 )
  {
    int actual_num = clamp( num, 0, COMBO_POINTS_MAX - count );
    int overflow   = num - actual_num;

    // we count all combo points gained in the proc
    for ( int i = 0; i < num; i++ )
      proc -> occur();

    // add actual combo points
    if ( actual_num > 0 )
      count += actual_num;

    // count wasted combo points
    if ( overflow > 0 )
    {
      for ( int i = 0; i < overflow; i++ )
        wasted -> occur();
    }

    if ( sim -> log )
    {
      if ( action )
        sim -> output( "%s gains %d (%d) combo_points from %s (%d)",
                       player -> name(), actual_num, num, action, count );
      else
        sim -> output( "%s gains %d (%d) combo_points (%d)",
                       player -> name(), actual_num, num, count );
    }
  }

  void clear( const char* action = 0 )
  {
    if ( sim -> log )
    {
      if ( action )
        sim -> output( "%s spends %d combo_points on %s",
                       player -> name(), count, action );
      else
        sim -> output( "%s loses %d combo_points",
                       player -> name(), count );
    }

    count = 0;
  }

  double rank( double* cp_list ) const
  {
    assert( count > 0 );
    return cp_list[ count - 1 ];
  }

  double rank( double cp1, double cp2, double cp3, double cp4, double cp5 ) const
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return rank( cp_list );
  }
};

// ==========================================================================
// Rogue
// ==========================================================================

// ==========================================================================
//  New Ability: Redirect, doable once we allow debuffs on multiple targets
//  Review: Ability Scaling
//  Review: Energy Regen (how Adrenaline rush stacks with Blade Flurry / haste)
// ==========================================================================

enum poison_e { POISON_NONE=0, DEADLY_POISON, WOUND_POISON, CRIPPLING_POISON, MINDNUMBING_POISON, LEECHING_POISON, PARALYTIC_POISON };

struct rogue_td_t : public actor_pair_t
{
  dot_t* dots_crimson_tempest;
  dot_t* dots_deadly_poison;
  dot_t* dots_garrote;
  dot_t* dots_hemorrhage;
  dot_t* dots_rupture;

  buff_t* debuffs_anticipation_charges;
  buff_t* debuffs_find_weakness;
  buff_t* debuffs_vendetta;
  buff_t* debuffs_revealing_strike;

  combo_points_t* combo_points;

  rogue_td_t( player_t* target, rogue_t* source );

  void reset()
  {
    combo_points -> clear();
  }

  ~rogue_td_t()
  {
    delete combo_points;
  }
  
  bool poisoned()
  {
    return dots_deadly_poison -> ticking == 1;
  }
};

struct rogue_t : public player_t
{
  // Active
  action_t* active_lethal_poison;

  action_t* active_main_gauche;
  action_t* active_venomous_wound;

  // Benefits
  struct benefits_t
  {
    benefit_t* poisoned;
  } benefits;

  // Buffs
  struct buffs_t
  {
    buff_t* adrenaline_rush;
    buff_t* bandits_guile;
    buff_t* blade_flurry;
    buff_t* blindside;
    buff_t* deadly_poison;
    buff_t* deadly_proc;
    buff_t* deep_insight;
    buff_t* envenom;
    buff_t* leeching_poison;
    buff_t* killing_spree;
    buff_t* master_of_subtlety;
    buff_t* moderate_insight;
    buff_t* recuperate;
    buff_t* shadow_dance;
    buff_t* shadowstep;
    buff_t* shallow_insight;
    buff_t* shiv;
    buff_t* slice_and_dice;
    buff_t* stealthed;
    buff_t* tier13_2pc;
    buff_t* tot_trigger;
    buff_t* vanish;
    buff_t* wound_poison;

    // Legendary buffs
    buff_t* fof_fod; // Fangs of the Destroyer
    stat_buff_t* fof_p1; // Phase 1
    stat_buff_t* fof_p2;
    stat_buff_t* fof_p3;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* honor_among_thieves;
    cooldown_t* killing_spree;
    cooldown_t* seal_fate;
    cooldown_t* shadow_blades;
  } cooldowns;

  // Expirations
  struct expirations_t_
  {
    event_t* wound_poison;

    void reset() { memset( ( void* ) this, 0x00, sizeof( expirations_t_ ) ); }
    expirations_t_() { reset(); }
  };
  expirations_t_ expirations_;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* combat_potency;
    gain_t* energy_refund;
    gain_t* energetic_recovery;
    gain_t* murderous_intent;
    gain_t* overkill;
    gain_t* recuperate;
    gain_t* relentless_strikes;
    gain_t* vitality;
    gain_t* venomous_wounds;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Assassination
    const spell_data_t* assassins_resolve;
    const spell_data_t* improved_poisons;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;
    const spell_data_t* cut_to_the_chase;
    const spell_data_t* blindside;
    
    // Combat
    const spell_data_t* ambidexterity;
    const spell_data_t* vitality;
    const spell_data_t* combat_potency;
    const spell_data_t* restless_blades;
    const spell_data_t* bandits_guile;
    const spell_data_t* killing_spree;
    
    // Subtlety
    const spell_data_t* master_of_subtlety;
    const spell_data_t* sinister_calling;
    const spell_data_t* find_weakness;
    const spell_data_t* honor_among_thieves;
    const spell_data_t* sanguinary_vein;
    const spell_data_t* energetic_recovery;
    const spell_data_t* shadow_dance;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* bandits_guile_value;
    const spell_data_t* master_poisoner;
    const spell_data_t* relentless_strikes;
    const spell_data_t* tier13_2pc;
    const spell_data_t* tier13_4pc;
  } spell;

  // Talents
  struct talents_t
  {
  } talents;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* executioner;
    const spell_data_t* main_gauche;
    const spell_data_t* potent_poisons;
  } mastery;

  struct glyphs_t
  {
    const spell_data_t* kick;
  } glyph;

  // Procs
  struct procs_t
  {
    proc_t* deadly_poison;
    proc_t* wound_poison;
    proc_t* honor_among_thieves;
    proc_t* main_gauche;
    proc_t* seal_fate;
    proc_t* venomous_wounds;
  } procs;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* combat_potency;
    rng_t* deadly_poison;
    rng_t* hat_interval;
    rng_t* honor_among_thieves;
    rng_t* main_gauche;
    rng_t* revealing_strike;
    rng_t* relentless_strikes;
    rng_t* venomous_wounds;
    rng_t* wound_poison;
  } rng;

  action_callback_t* virtual_hat_callback;

  player_t* tot_target;

  // Options
  timespan_t virtual_hat_interval;
  std::string tricks_of_the_trade_target_str;

  uint32_t fof_p1, fof_p2, fof_p3;

private:
  target_specific_t<rogue_td_t> target_data;
public:

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    active_main_gauche( 0 ), active_venomous_wound( 0 ),
    benefits( benefits_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    virtual_hat_callback( 0 ),
    tot_target( 0 ),
    virtual_hat_interval( timespan_t::min() ),
    tricks_of_the_trade_target_str( "" ),
    fof_p1( 0 ), fof_p2( 0 ), fof_p3( 0 )
  {
    target_data.init( "target_data", this );

    // Cooldowns
    cooldowns.honor_among_thieves = get_cooldown( "honor_among_thieves" );
    cooldowns.seal_fate           = get_cooldown( "seal_fate"           );
    cooldowns.adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns.killing_spree       = get_cooldown( "killing_spree"       );
    cooldowns.shadow_blades       = get_cooldown( "shadow_blades"       );

    initial.distance = 3;

  }

  // Character Definition
  virtual rogue_td_t* get_target_data( player_t* target )
  {
    rogue_td_t*& td = target_data[ target ];
    if ( ! td ) td = new rogue_td_t( target, this );
    return td;
  }

  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_actions();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      arise();
  virtual double    energy_regen_per_second();
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_ENERGY; }
  virtual role_e primary_role()  { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, save_e=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );

  virtual double    composite_attribute_multiplier( attribute_e attr );
  virtual double    composite_attack_speed();
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual double    composite_attack_power_multiplier();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_state_t : public action_state_t
{
  int combo_points;

  rogue_attack_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ), combo_points( 0 )
  { }

  virtual void debug()
  {
    action_state_t::debug();
    action -> sim -> output( "[NEW] %s %s %s: cp=%d",
                             action -> player -> name(),
                             action -> name(),
                             target -> name(),
                             combo_points );
  }

  virtual void copy_state( const action_state_t* o )
  {
    if ( o == 0 || this == o )
      return;

    action_state_t::copy_state( o );

    const rogue_attack_state_t* ds_ = static_cast< const rogue_attack_state_t* >( o );

    combo_points = ds_ -> combo_points;
  }
};

struct rogue_melee_attack_t : public melee_attack_t
{
  double      base_direct_damage;
  double      base_direct_power_mod;
  bool        requires_stealth_;
  position_e  requires_position_;
  bool        requires_combo_points;
  int         adds_combo_points;
  double      base_da_bonus;
  double      base_ta_bonus;
  weapon_e    requires_weapon;
  bool        affected_by_killing_spree;

  // we now track how much combo points we spent on an action
  int              combo_points_spent;

  rogue_melee_attack_t( const std::string& token, rogue_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        const std::string& options = std::string() ) :
    melee_attack_t( token, p, s ),
    base_direct_damage( 0 ), base_direct_power_mod( 0 ),
    requires_stealth_( false ), requires_position_( POSITION_NONE ),
    requires_combo_points( false ), adds_combo_points( 0 ),
    base_da_bonus( 0.0 ), base_ta_bonus( 0.0 ),
    requires_weapon( WEAPON_NONE ),
    affected_by_killing_spree( true ),
    combo_points_spent( 0 )
  {
    parse_options( 0, options );

    may_crit                  = true;
    may_glance                = false;
    special                   = true;
    tick_may_crit             = true;
    hasted_ticks              = false;

    for ( size_t i = 1; i <= data()._effects -> size(); i++ )
    {
      switch ( data().effectN( i ).type() )
      {
      case E_ADD_COMBO_POINTS:
        adds_combo_points = data().effectN( i ).base_value();
        break;
      default:
        break;
      }

      if ( data().effectN( i ).type() == E_APPLY_AURA &&
           data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
        base_ta_bonus = data().effectN( i ).bonus( p );
      else if ( data().effectN( i ).type() == E_SCHOOL_DAMAGE )
        base_da_bonus = data().effectN( i ).bonus( p );
    }
  }
  
  action_state_t* new_state()
  { return new rogue_attack_state_t( this, target ); }

  rogue_t* cast() const
  { return static_cast<rogue_t*>( player ); }

  rogue_t* p() const
  { return static_cast<rogue_t*>( player ); }

  rogue_td_t* cast_td( player_t* t = 0 ) const
  { return p() -> get_target_data( t ? t : target ); }

  virtual double cost();
  virtual void   execute();
  virtual void   consume_resource();
  virtual bool   ready();
  virtual void   impact( action_state_t* state );

  virtual double calculate_weapon_damage( double /* attack_power */ );
  virtual void   assess_damage( dmg_e, action_state_t* s );
  virtual double armor();

  virtual bool   requires_stealth() const
  {
    if ( p() -> buffs.shadow_dance -> check() ||
         p() -> buffs.stealthed -> check() ||
         p() -> buffs.vanish -> check() )
    {
      return false;
    }

    return requires_stealth_;
  }

  virtual position_e requires_position() const
  {
    return requires_position_;
  }

  action_state_t* get_state( const action_state_t* s )
  {
    action_state_t* s_ = melee_attack_t::get_state( s );

    if ( s == 0 )
    {
      rogue_attack_state_t* ds_ = static_cast< rogue_attack_state_t* >( s_ );
      ds_ -> combo_points = 0;
    }

    return s_;
  }
  
  virtual double composite_da_multiplier()
  {
    double m = melee_attack_t::composite_da_multiplier();

    if ( p() -> main_hand_weapon.type == WEAPON_DAGGER && p() -> off_hand_weapon.type == WEAPON_DAGGER )
      m *= 1.0 + p() -> spec.assassins_resolve -> effectN( 2 ).percent();

    // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
    // affects deadly poison ticks
    // Exception: DOESN'T affect rupture/garrote ticks
    if ( affected_by_killing_spree )
      m *= 1.0 + p() -> buffs.killing_spree -> value();

    return m;
  }
  
  virtual double composite_target_multiplier( player_t* target )
  {
    double m = melee_attack_t::composite_target_multiplier( target );

    rogue_td_t* td = cast_td( target );

    if ( requires_combo_points && td -> debuffs_revealing_strike -> up() )
      m *= 1.0 + td -> debuffs_revealing_strike -> value();

    m *= 1.0 + td -> debuffs_vendetta -> value();
    
    if ( p() -> spec.sanguinary_vein -> ok() && 
         ( td -> dots_garrote -> ticking || td -> dots_rupture -> ticking ) )
         m *= 1.0 + p() -> spec.sanguinary_vein -> effectN( 2 ).percent();

    return m;
  }
  
  virtual double composite_ta_multiplier()
  {
    double m = melee_attack_t::composite_ta_multiplier();

    if ( p() -> main_hand_weapon.type == WEAPON_DAGGER && p() -> off_hand_weapon.type == WEAPON_DAGGER )
      m *= 1.0 + p() -> spec.assassins_resolve -> effectN( 2 ).percent();

    return m;
  }
  
  virtual double action_multiplier()
  {
    double m = melee_attack_t::action_multiplier();

    rogue_t* p = cast();
    
    if ( requires_combo_points && p -> mastery.executioner -> ok() )
      m += p -> composite_mastery() * p -> mastery.executioner -> effectN( 1 ).mastery_value();

    return m;
  }

  // Combo points need to be snapshot before we travel, they should also not
  // be snapshot during any other event in the stateless system.
  void schedule_travel( action_state_t* travel_state )
  {
    if ( result_is_hit( travel_state -> result ) )
    {
      rogue_attack_state_t* ds_ = static_cast< rogue_attack_state_t* >( travel_state );
      ds_ -> combo_points = cast_td( travel_state -> target ) -> combo_points -> count;
    }

    melee_attack_t::schedule_travel( travel_state );
  }
};

// ==========================================================================
// Rogue Poison
// ==========================================================================

struct rogue_poison_t : public rogue_melee_attack_t
{
  rogue_poison_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    rogue_melee_attack_t( token, p, s )
  {
    proc              = true;
    background        = true;
    trigger_gcd       = timespan_t::zero();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;

    weapon_multiplier = 0;
  }

  virtual double action_da_multiplier()
  {
    double m = rogue_melee_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m += p() -> composite_mastery() * p() -> mastery.potent_poisons -> effectN( 1 ).mastery_value();

    return m;
  }

  virtual double action_ta_multiplier()
  {
    double m = rogue_melee_attack_t::action_ta_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m += p() -> composite_mastery() * p() -> mastery.potent_poisons -> effectN( 1 ).mastery_value();

    return m;
  }
};

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs.stealthed -> check() )
  {
    p -> buffs.stealthed -> expire();
    p -> buffs.master_of_subtlety -> trigger();
  }

  if ( p -> buffs.vanish -> check() )
  {
    p -> buffs.vanish -> expire();
    p -> buffs.master_of_subtlety -> trigger();
  }
}

// trigger_combat_potency ===================================================

static void trigger_combat_potency( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spec.combat_potency -> ok() )
    return;

  if ( p -> rng.combat_potency -> roll( p -> spec.combat_potency -> proc_chance() ) )
  {
    // energy gain value is in the proc trigger spell
    p -> resource_gain( RESOURCE_ENERGY,
                        p -> spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                        p -> gains.combat_potency );
  }
}

// trigger_energy_refund ====================================================

static void trigger_energy_refund( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains.energy_refund );
}

// trigger_relentless_strikes ===============================================

static void trigger_relentless_strikes( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spell.relentless_strikes -> ok() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  rogue_td_t* td = a -> cast_td();
  double chance = p -> spell.relentless_strikes -> effectN( 1 ).pp_combo_points() / 100.0;
  if ( p -> rng.relentless_strikes -> roll( chance * td -> combo_points -> count ) )
  {
    double gain = p -> spell.relentless_strikes -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY );
    p -> resource_gain( RESOURCE_ENERGY, gain, p -> gains.relentless_strikes );
  }
}

// trigger_restless_blades ==================================================

static void trigger_restless_blades( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> spec.restless_blades -> ok() )
    return;

  if ( ! a -> requires_combo_points )
    return;
  
  rogue_attack_state_t* state = static_cast< rogue_attack_state_t* >( a -> execute_state );
  timespan_t reduction = p -> spec.restless_blades -> effectN( 1 ).time_value() * state -> combo_points;

  p -> cooldowns.adrenaline_rush -> ready -= reduction;
  p -> cooldowns.killing_spree -> ready   -= reduction;
  p -> cooldowns.shadow_blades -> ready   -= reduction;
}

// trigger_seal_fate ========================================================

void trigger_seal_fate( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> p();

  if ( ! p -> spec.seal_fate -> ok() )
    return;

  if ( ! a -> adds_combo_points )
    return;

  if ( a -> aoe != 0 )
    return;

  // This is to prevent dual-weapon special attacks from triggering a double-proc of Seal Fate
  if ( p -> cooldowns.seal_fate -> remains() > timespan_t::zero() )
    return;

  rogue_td_t* td = a -> cast_td();
  td -> combo_points -> add( 1, p -> spec.seal_fate -> name_cstr() );

  p -> cooldowns.seal_fate -> start( timespan_t::from_millis( 1 ) );
  p -> procs.seal_fate -> occur();
}

// trigger_main_gauche ======================================================

static void trigger_main_gauche( rogue_melee_attack_t* a )
{
  struct main_gauche_t : public rogue_melee_attack_t
  {
    main_gauche_t( rogue_t* p ) : 
      rogue_melee_attack_t( "main_gauche", p, p -> find_spell( 86392 ) )
    {
      weapon          = &( p -> main_hand_weapon );
      special         = true;
      background      = true;
      trigger_gcd     = timespan_t::zero();
      may_crit        = true;
      proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs
    }

    virtual void impact( action_state_t* state )
    {
      rogue_melee_attack_t::impact( state );
      
      if ( result_is_hit( state -> result ) )
        trigger_combat_potency( this );
    }
  };

  if ( a -> proc )
    return;

  weapon_t* w = a -> weapon;

  if ( ! w || w -> slot != SLOT_MAIN_HAND )
    return;

  rogue_t* p = a -> cast();

  if ( ! p -> mastery.main_gauche -> ok() )
    return;

  double chance = p -> composite_mastery() * p -> mastery.main_gauche -> effectN( 1 ).mastery_value();

  if ( p -> rng.main_gauche -> roll( chance ) )
  {
    if ( ! p -> active_main_gauche )
      p -> active_main_gauche = new main_gauche_t( p );

    p -> active_main_gauche -> execute();
    p -> procs.main_gauche  -> occur();
  }
}

// trigger_tricks_of_the_trade ==============================================

static void trigger_tricks_of_the_trade( rogue_melee_attack_t* a )
{
  rogue_t* p = a -> cast();

  if ( ! p -> buffs.tot_trigger -> check() )
    return;

  player_t* t = p -> tot_target;

  if ( t )
  {
    timespan_t duration = p -> dbc.spell( 57933 ) -> duration();

    if ( t -> buffs.tricks_of_the_trade -> remains_lt( duration ) )
    {
      double value = p -> dbc.spell( 57933 ) -> effectN( 1 ).base_value();

      t -> buffs.tricks_of_the_trade -> buff_duration = duration;
      t -> buffs.tricks_of_the_trade -> trigger( 1, value / 100.0 );
    }

    p -> buffs.tot_trigger -> expire();
  }
}

// trigger_venomous_wounds ==================================================

static void trigger_venomous_wounds( rogue_melee_attack_t* a )
{
  struct venomous_wound_t : public rogue_poison_t
  {
    venomous_wound_t( rogue_t* p ) :
      rogue_poison_t( "venomous_wound", p, p -> find_spell( 79136 ) )
    {
      background       = true;
      proc             = true;
      direct_power_mod = data().extra_coeff();
    }
  };

  rogue_t* p = a -> cast();

  if ( ! p -> spec.venomous_wounds -> ok() )
    return;

  if ( p -> rng.venomous_wounds -> roll( p -> spec.venomous_wounds -> proc_chance() ) )
  {
    if ( ! p -> active_venomous_wound )
    {
      p -> active_venomous_wound = new venomous_wound_t( p );
      p -> active_venomous_wound -> init();
    };

    p -> procs.venomous_wounds -> occur();
    p -> active_venomous_wound -> execute();

    p -> resource_gain( RESOURCE_ENERGY,
                        p -> spec.venomous_wounds -> effectN( 2 ).base_value(),
                        p -> gains.venomous_wounds );
  }
}

// ==========================================================================
// Attacks
// ==========================================================================

// rogue_melee_attack_t::impact =============================================

void rogue_melee_attack_t::impact( action_state_t* state )
{
  melee_attack_t::impact( state );
  
  if ( result_is_hit( state -> result ) )
  {
    if ( weapon && ! requires_combo_points && p() -> active_lethal_poison )
      p() -> active_lethal_poison -> execute();

    if ( state -> result == RESULT_CRIT )
      trigger_seal_fate( this );
  }
}

// rogue_melee_attack_t::armor ==============================================

double rogue_melee_attack_t::armor()
{
  double a = melee_attack_t::armor();
  
  rogue_td_t* td = cast_td();

  // FIXME armor needs stateless handling, in theory?
  a *= 1.0 - td -> debuffs_find_weakness -> value();

  return a;
}

// rogue_melee_attack_t::cost ===============================================

double rogue_melee_attack_t::cost()
{
  double c = melee_attack_t::cost();

  rogue_t* p = cast();

  if ( c <= 0 )
    return 0;

  if ( p -> set_bonus.tier13_2pc_melee() && p -> buffs.tier13_2pc -> up() )
    c *= 1.0 + p -> spell.tier13_2pc -> effectN( 1 ).percent();

  return c;
}

// rogue_melee_attack_t::consume_resource =========================================

void rogue_melee_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();

  rogue_t* p = cast();

  combo_points_spent = 0;

  if ( result_is_hit( execute_state -> result ) )
  {
    trigger_relentless_strikes( this );

    if ( requires_combo_points )
    {
      rogue_td_t* td = cast_td();
      combo_points_spent = td -> combo_points -> count;

      td -> combo_points -> clear( name() );

      if ( p -> buffs.fof_fod -> up() )
        td -> combo_points -> add( 5, "legendary_daggers" );
    }
  }
  else
    trigger_energy_refund( this );
}

// rogue_melee_attack_t::execute ==================================================

void rogue_melee_attack_t::execute()
{
  rogue_t* p = cast();
  rogue_td_t* td = cast_td();

  if ( harmful )
    break_stealth( p );

  if ( requires_combo_points && base_direct_power_mod > 0 )
    direct_power_mod = base_direct_power_mod * td -> combo_points -> count;
  
  if ( requires_combo_points && base_dd_min > 0 )
    base_dd_min      = base_dd_max = base_direct_damage * td -> combo_points -> count;

  melee_attack_t::execute();

  if ( result_is_hit( execute_state -> result ) )
  {
    if ( adds_combo_points )
      td -> combo_points -> add( adds_combo_points, name() );

    if ( may_crit )  // Rupture and Garrote can't proc MG, issue 581
      trigger_main_gauche( this );

    trigger_tricks_of_the_trade( this );

    trigger_restless_blades( this );
  }
}

// rogue_melee_attack_t::calculate_weapon_damage ==================================

double rogue_melee_attack_t::calculate_weapon_damage( double attack_power )
{
  double dmg = melee_attack_t::calculate_weapon_damage( attack_power );

  if ( dmg == 0 ) return 0;

  rogue_t* p = cast();

  if ( weapon -> slot == SLOT_OFF_HAND && p -> spec.ambidexterity -> ok() )
  {
    dmg *= 1.0 + p -> spec.ambidexterity -> effectN( 1 ).percent();
  }

  return dmg;
}

// rogue_melee_attack_t::ready() ==================================================

bool rogue_melee_attack_t::ready()
{
  rogue_t* p = cast();

  if ( requires_combo_points )
  {
    rogue_td_t* td = cast_td();
    if ( ! td -> combo_points -> count )
      return false;
  }

  if ( requires_stealth() )
  {
    if ( ! p -> buffs.shadow_dance -> check() &&
         ! p -> buffs.stealthed -> check() &&
         ! p -> buffs.vanish -> check() )
    {
      return false;
    }
  }

  //Killing Spree blocks all rogue actions for duration
  if ( p -> buffs.killing_spree -> check() )
    if ( ( special == true ) && ( proc == false ) )
      return false;

  if ( requires_position() != POSITION_NONE )
    if ( p -> position != requires_position() )
      return false;

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  return melee_attack_t::ready();
}

// rogue_melee_attack_t::assess_damage ============================================

void rogue_melee_attack_t::assess_damage( dmg_e type,
                                          action_state_t* s)
{
  melee_attack_t::assess_damage( type, s );

  /*rogue_t* p = cast();

  // XXX: review, as not all of the damage is 'flurried' to an additional target
  // dots for example don't as far as I remember
  if ( t -> is_enemy() )
  {
    target_t* q = t -> cast_target();

    if ( p -> buffs.blade_flurry -> up() && q -> adds_nearby )
      melee_attack_t::additional_damage( q, amount, dmg_e, impact_result );
  }*/
}


// Melee Attack =============================================================

struct melee_t : public rogue_melee_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_melee_attack_t( name, p ), sync_weapons( sw ), first( true )
  {
    school          = SCHOOL_PHYSICAL;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    may_crit        = true;

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  void reset()
  {
    rogue_melee_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = rogue_melee_attack_t::execute_time();
    if ( first )
    {
      first = false;
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.01 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  virtual void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( weapon -> slot == SLOT_OFF_HAND )
        trigger_combat_potency( this );

      rogue_t* p = cast();

      // Legendary Daggers buff handling
      // Proc rates from: https://github.com/Aldriana/ShadowCraft-Engine/blob/master/shadowcraft/objects/proc_data.py#L504
      // Logic from: http://code.google.com/p/simulationcraft/issues/detail?id=1118
      double fof_chance = ( p -> specialization() == ROGUE_ASSASSINATION ) ? 0.235 : ( p -> specialization() == ROGUE_COMBAT ) ? 0.095 : 0.275;
      if ( sim -> roll( fof_chance ) )
      {
        p -> buffs.fof_p1 -> trigger();
        p -> buffs.fof_p2 -> trigger();

        if ( ! p -> buffs.fof_fod -> check() && p -> buffs.fof_p3 -> check() > 30 )
        {
          // Trigging FoF and the Stacking Buff are mutually exclusive
          if ( sim -> roll( 1.0 / ( 50.0 - p -> buffs.fof_p3 -> check() ) ) )
          {
            p -> buffs.fof_fod -> trigger();
            rogue_td_t* td = cast_td();
            td -> combo_points -> add( 5, "legendary_daggers" );
          }
          else
          {
            p -> buffs.fof_p3 -> trigger();
          }
        }
        else
        {
          p -> buffs.fof_p3 -> trigger();
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public action_t
{
  int sync_weapons;

  auto_melee_attack_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    player -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      player -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( player -> is_moving() )
      return false;

    return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_melee_attack_t
{
  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "adrenaline_rush", p, p -> find_class_spell( "Adrenaline Rush" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }
  
  void execute()
  {
    rogue_melee_attack_t::execute();
    
    p() -> buffs.adrenaline_rush -> trigger();
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_melee_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "ambush", p, p -> find_class_spell( "Ambush" ), options_str )
  {
    requires_position_      = POSITION_BACK;
    requires_stealth_       = true;

    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier   *= 1.447; // It'is in the description.
  }
  
  virtual double cost()
  {
    double c = rogue_melee_attack_t::cost();
    
    if ( p() -> buffs.shadow_dance -> check() )
      c += p() -> spec.shadow_dance -> effectN( 2 ).base_value();
    
    return c;
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    p() -> buffs.shadowstep -> expire();
  }
  
  void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );
    
    if ( result_is_hit( state -> result ) )
    {
      rogue_td_t* td = cast_td();
      
      td -> debuffs_find_weakness -> trigger();
    }
  }

  virtual double action_multiplier()
  {
    double am = rogue_melee_attack_t::action_multiplier();

    am *= 1.0 + p() -> buffs.shadowstep -> value();

    return am;
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_melee_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "backstab", p, p -> find_class_spell( "Backstab" ), options_str )
  {
    requires_weapon     = WEAPON_DAGGER;
    requires_position_  = POSITION_BACK;
  }
};

// Dispatch =================================================================

struct dispatch_t : public rogue_melee_attack_t
{
  dispatch_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "dispatch", p, p -> find_class_spell( "Dispatch" ), options_str )
  {
    if ( p -> main_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Trying to use %s without a dagger in main-hand", name() );
      background = true;
    }
  }

  double cost()
  {
    if ( p() -> buffs.blindside -> check() )
      return 0;
    else
      return rogue_melee_attack_t::cost();
  }
  
  void execute()
  {
    rogue_melee_attack_t::execute();

    p() -> buffs.blindside -> expire();
  }

  bool ready()
  {
    if ( ! p() -> buffs.blindside -> check() && target -> health_percentage() > 0.35 )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_melee_attack_t
{
  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "envenom", p, p -> find_class_spell( "Envenom" ), options_str )
  {
    requires_combo_points = true;
    // FIXME: Envenom tooltip in 15882 is off, this should be more accurate
    base_direct_damage    = p -> dbc.spell_scaling( p -> type, p -> level ) * 0.213323765;
    base_direct_power_mod = 0.112;
    num_ticks             = 0;
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    rogue_td_t* td = cast_td();

    p -> buffs.envenom -> trigger( 1, -1.0, -1.0, 
      timespan_t::from_seconds( 1 + td -> combo_points -> count ) );

    rogue_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_restless_blades( this );
  }
  
  virtual double action_da_multiplier()
  {
    double m = rogue_melee_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m += p() -> composite_mastery() * p() -> mastery.potent_poisons -> effectN( 1 ).mastery_value();

    return m;
  }

  virtual void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );

    if ( p() -> spec.cut_to_the_chase -> ok() )
    {
      double sld = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
      sld *= 1.0 + p() -> mastery.executioner -> effectN( 1 ).mastery_value() * p() -> composite_mastery();

      p() -> buffs.slice_and_dice -> trigger( 1, sld, -1.0, 
        p() -> buffs.slice_and_dice -> data().duration() * ( COMBO_POINTS_MAX + 1 ) );
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_melee_attack_t
{
  double combo_point_dmg_min[ COMBO_POINTS_MAX ];
  double combo_point_dmg_max[ COMBO_POINTS_MAX ];

  eviscerate_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "eviscerate", p, p -> find_class_spell( "Eviscerate" ), options_str )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    requires_combo_points = true;

    double min_     = data().effectN( 1 ).min( p );
    double max_     = data().effectN( 1 ).max( p );
    double cp_bonus = data().effectN( 1 ).bonus( p );

    for ( int i = 0; i < COMBO_POINTS_MAX; i++ )
    {
      combo_point_dmg_min[ i ] = min_ + cp_bonus * ( i + 1 );
      combo_point_dmg_max[ i ] = max_ + cp_bonus * ( i + 1 );
    }
  }

  virtual void execute()
  {
    rogue_td_t* td = cast_td();

    base_dd_min = td -> combo_points -> rank( combo_point_dmg_min );
    base_dd_max = td -> combo_points -> rank( combo_point_dmg_max );

    direct_power_mod = 0.16 * td -> combo_points -> count;

    rogue_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_restless_blades( this );
  }
  
  virtual void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );
    
    double sld = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
    sld *= 1.0 + p() -> mastery.executioner -> effectN( 1 ).mastery_value() * p() -> composite_mastery();

    p() -> buffs.slice_and_dice -> trigger( 1, sld, -1.0, 
      p() -> buffs.slice_and_dice -> data().duration() * ( COMBO_POINTS_MAX + 1 ) );
  }
};

// Expose Armor =============================================================

struct expose_armor_t : public rogue_melee_attack_t
{
  expose_armor_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "expose_armor", p, p -> find_class_spell( "Expose Armor" ), options_str )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( ! sim -> overrides.weakened_armor )
        target -> debuffs.weakened_armor -> trigger();

      rogue_td_t* td = cast_td();
      td -> combo_points -> add( 1 );
    }
  };
};

// Fan of Knives ============================================================

struct fan_of_knives_t : public rogue_melee_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "fan_of_knives", p, p -> find_class_spell( "Fan of Knives" ), options_str )
  {
    aoe    = -1;
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_melee_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "garrote", p, p -> find_class_spell( "Garrote" ), options_str )
  {
    affected_by_killing_spree = false;

    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    // to stop seal fate procs
    may_crit          = false;

    requires_position_ = POSITION_BACK;
    requires_stealth_  = true;

    tick_power_mod    = .07;
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    p() -> buffs.shadowstep -> expire();
  }
  
  void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      rogue_td_t* td = cast_td();
      
      td -> debuffs_find_weakness -> trigger();
    }
  }

  virtual double action_multiplier()
  {
    double am = rogue_melee_attack_t::action_multiplier();

    rogue_t* p = cast();

    if ( p -> buffs.shadowstep -> check() )
      am *= 1.0 + p -> buffs.shadowstep -> value();

    return am;
  }

  virtual void tick( dot_t* d )
  {
    rogue_melee_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_melee_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "hemorrhage", p, p -> find_class_spell( "Hemorrhage" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier *= 1.45; // number taken from spell description

    base_tick_time = p -> find_spell( 89975 ) -> effectN( 1 ).period();
    num_ticks = p -> find_spell( 89775 ) -> duration().total_seconds() / base_tick_time.total_seconds();
    dot_behavior = DOT_REFRESH;
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      base_td = execute_state -> result_amount * data().effectN( 3 ).percent();
  }
};

// Kick =====================================================================

struct kick_t : public rogue_melee_attack_t
{
  kick_t( rogue_t* p, const std::string& options_str ) :
  rogue_melee_attack_t( "kick", p, p -> find_class_spell( "Kick" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    if ( p -> glyph.kick -> ok() )
    {
      // All kicks are assumed to interrupt a cast
      cooldown -> duration -= timespan_t::from_seconds( 2.0 ); // + 4 Duration - 6 on Success = -2
    }
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_melee_attack_t
{
  killing_spree_tick_t( rogue_t* p, const char* name ) :
    rogue_melee_attack_t( name, p, spell_data_t::nil() )
  {
    background  = true;
    may_crit    = true;
    direct_tick = true;
    normalize_weapon_speed = true;
  }
};

struct killing_spree_t : public rogue_melee_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "killing_spree", p, p -> find_class_spell( "Killing Spree" ), options_str ),
    attack_mh( 0 ), attack_oh( 0 )
  {
    num_ticks = 7;
    may_crit  = false;
    channeled = true;

    attack_mh = new killing_spree_tick_t( p, "killing_spree_mh" );
    attack_mh -> weapon = &( player -> main_hand_weapon );
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh" );
      attack_oh -> weapon = &( player -> off_hand_weapon );
      add_child( attack_oh );
    }
  }

  virtual void tick( dot_t* d )
  {
    rogue_melee_attack_t::tick( d );

    attack_mh -> execute();

    if ( attack_oh && result_is_hit( attack_mh -> execute_state -> result ) )
      attack_oh -> execute();
  }
};

// Mutilate =================================================================

struct mutilate_strike_t : public rogue_melee_attack_t
{
  mutilate_strike_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_melee_attack_t( name, p, s )
  {
    background  = true;
    may_miss = may_dodge = may_parry = false;
  }
};

struct mutilate_t : public rogue_melee_attack_t
{
  rogue_melee_attack_t* mh_strike;
  rogue_melee_attack_t* oh_strike;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "mutilate", p, p -> find_class_spell( "Mutilate" ), options_str ),
    mh_strike( 0 ), oh_strike( 0 )
  {
    may_crit = false;

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      background = true;
    }

    mh_strike = new mutilate_strike_t( p, "mutilate_mh", data().effectN( 2 ).trigger() );
    mh_strike -> weapon = &( p -> main_hand_weapon );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh", data().effectN( 3 ).trigger() );
    oh_strike -> weapon = &( p -> off_hand_weapon );
    add_child( oh_strike );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      mh_strike -> execute();
      oh_strike -> execute();
    }
  }
  
  virtual void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
      p() -> buffs.blindside -> trigger();
  }
};

// Premeditation ============================================================

// FIXME: Implement somehow
struct premeditation_t : public rogue_melee_attack_t
{
  premeditation_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "premeditation", p, p -> find_class_spell( "Premeditation" ), options_str )
  {
    harmful = false;
  }
};

// Recuperate ===============================================================

struct recuperate_t : public rogue_melee_attack_t
{
  recuperate_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "recuperate", p, p -> find_class_spell( "Recuperate" ), options_str )
  {
    requires_combo_points = true;
    dot_behavior = DOT_REFRESH;
    harmful = false;
  }

  virtual void execute()
  {
    rogue_t* p = cast();
    rogue_td_t* td = cast_td();
    num_ticks = 2 * td -> combo_points -> count;
    rogue_melee_attack_t::execute();
    p -> buffs.recuperate -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    rogue_t* p = cast();
    p -> buffs.recuperate -> expire();
    rogue_melee_attack_t::last_tick( d );
  }
};

// Revealing Strike =========================================================

struct revealing_strike_t : public rogue_melee_attack_t
{
  revealing_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "revealing_strike", p, p -> find_class_spell( "Revealing Strike" ), options_str )
  {
    // Legendary buff increases RS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.0 + p -> dbc.spell( 110211 ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      p() -> buffs.bandits_guile -> trigger();

      rogue_td_t* td = cast_td( state -> target );
      td -> debuffs_revealing_strike -> trigger();
    }
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_melee_attack_t
{
  double combo_point_base_td;
  double combo_point_tick_power_mod[ COMBO_POINTS_MAX ];

  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "rupture", p, p -> find_class_spell( "Rupture" ), options_str )
  {
    affected_by_killing_spree = false;

    combo_point_tick_power_mod[ 0 ] = 0.025;
    combo_point_tick_power_mod[ 1 ] = 0.04;
    combo_point_tick_power_mod[ 2 ] = 0.05;
    combo_point_tick_power_mod[ 3 ] = 0.056;
    combo_point_tick_power_mod[ 4 ] = 0.062;

    may_crit                   = false;
    requires_combo_points      = true;
    tick_may_crit              = true;
    dot_behavior               = DOT_REFRESH;
    combo_point_base_td        = data().effectN( 1 ).average( p );
    base_multiplier           += p -> spec.sanguinary_vein -> effectN( 1 ).percent();
  }
  
  virtual void execute()
  {
    rogue_td_t* td = cast_td();

    base_td = ( combo_point_base_td + base_ta_bonus * td -> combo_points -> count );
    tick_power_mod = combo_point_tick_power_mod[ td -> combo_points -> count - 1 ];
    num_ticks = 2 + td -> combo_points -> count * 2; 

    rogue_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_restless_blades( this );
  }

  virtual void tick( dot_t* d )
  {
    rogue_melee_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_melee_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "shadowstep", p, p -> find_class_spell( "Shadowstep" ), options_str )
  {
    harmful = false;
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_melee_attack_t
{
  shiv_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "shiv", p, p -> find_class_spell( "Shiv" ), options_str )
  {
    weapon = &( p -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE )
      background = true; // Do not allow execution.

    may_crit          = false;
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    p -> buffs.shiv -> trigger();
    rogue_melee_attack_t::execute();
    p -> buffs.shiv -> expire();
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_melee_attack_t
{
  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "sinister_strike", p, p -> find_class_spell( "Sinister Strike" ), options_str )
  {
    adds_combo_points = 1; // it has an effect but with no base value :rollseyes:

    // Legendary buff increases SS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.0 + p -> dbc.spell( 110211 ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* state )
  {
    rogue_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      p() -> buffs.bandits_guile -> trigger();

      rogue_td_t* td = cast_td( state -> target );
      if ( td -> debuffs_revealing_strike -> up() && 
           p() -> rng.revealing_strike -> roll( td -> debuffs_revealing_strike -> data().proc_chance() ) )
      {
        td -> combo_points -> add( 1, "revealing_strike" );
      }
    }
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_melee_attack_t
{
  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "slice_and_dice", p, p -> find_class_spell( "Slice and Dice" ), options_str )
  {
    requires_combo_points = true;
    harmful               = false;
    num_ticks             = 0;
  }

  virtual void execute()
  {
    rogue_td_t* td = cast_td();
    int action_cp = td -> combo_points -> count;

    rogue_melee_attack_t::execute();

    double sld = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
    sld *= 1.0 + p() -> mastery.executioner -> effectN( 1 ).mastery_value() * p() -> composite_mastery();

    p() -> buffs.slice_and_dice -> trigger( 1, sld, -1.0, timespan_t::from_seconds( 6 * ( action_cp + 1 ) ) );
  }
};

// Pool Energy ==============================================================

struct pool_energy_t : public action_t
{
  timespan_t wait;
  int for_next;
  action_t* next_action;

  pool_energy_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "pool_energy", p ),
    wait( timespan_t::from_seconds( 0.5 ) ), for_next( 0 ),
    next_action( 0 )
  {
    option_t options[] =
    {
      { "wait",     OPT_TIMESPAN, &wait     },
      { "for_next", OPT_BOOL,     &for_next },
      { 0,              OPT_UNKNOWN,  0 }
    };
    parse_options( options, options_str );
  }

  virtual void init()
  {
    action_t::init();

    if ( for_next )
    {
      std::vector<action_t*>::iterator pos = range::find( player -> active_action_list -> foreground_action_list, this );
      assert( pos != player -> active_action_list -> foreground_action_list.end() );
      if ( ++pos != player -> active_action_list -> foreground_action_list.end() )
        next_action = *pos;
      else
      {
        sim -> errorf( "%s: can't find next action.\n", __FUNCTION__ );
        sim -> cancel();
      }
    }
  }

  virtual void execute()
  {
    if ( sim -> log )
      sim -> output( "%s performs %s", player -> name(), name() );
  }

  virtual timespan_t gcd()
  {
    return wait;
  }

  virtual bool ready()
  {
    if ( next_action )
    {
      if ( next_action -> ready() )
        return false;

      // If the next action in the list would be "ready" if it was not constrained by energy,
      // then this command will pool energy until we have enough.

      player -> resources.current[ RESOURCE_ENERGY ] += 100;

      bool energy_limited = next_action -> ready();

      player -> resources.current[ RESOURCE_ENERGY ] -= 100;

      if ( ! energy_limited )
        return false;
    }

    return action_t::ready();
  }
};

// Preparation ==============================================================

struct preparation_t : public rogue_melee_attack_t
{
  std::vector<cooldown_t*> cooldown_list;

  preparation_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "preparation", p, p -> find_class_spell( "Preparation" ), options_str )
  {
    harmful = false;

    cooldown_list.push_back( p -> get_cooldown( "vanish"     ) );
  }

  virtual void execute()
  {
    rogue_melee_attack_t::execute();

    int num_cooldowns = ( int ) cooldown_list.size();
    for ( int i = 0; i < num_cooldowns; i++ )
      cooldown_list[ i ] -> reset();
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_melee_attack_t
{
  shadow_dance_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "shadow_dance", p, p -> find_class_spell( "Shadow Dance" ), options_str )
  {
    if ( p -> set_bonus.tier13_4pc_melee() )
      p -> buffs.shadow_dance -> buff_duration = data().duration() + p -> spell.tier13_4pc -> effectN( 1 ).time_value();
  }
};

// Tricks of the Trade ======================================================

struct tricks_of_the_trade_t : public rogue_melee_attack_t
{
  tricks_of_the_trade_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "tricks_of_the_trade", p, p -> find_class_spell( "Tricks of the Trade" ), options_str )
  {
    if ( ! p -> tricks_of_the_trade_target_str.empty() )
    {
      target_str = p -> tricks_of_the_trade_target_str;
    }

    if ( target_str.empty() )
    {
      target = p;
    }
    else
    {
      if ( target_str == "self" ) // This is only for backwards compatibility
      {
        target = p;
      }
      else
      {
        player_t* q = sim -> find_player( target_str );

        if ( q )
          target = q;
        else
        {
          sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
          target = p;
        }
      }
    }

    p -> tot_target = target;
  }

  virtual void execute()
  {
    rogue_t* p = cast();

    rogue_melee_attack_t::execute();

    p -> buffs.tier13_2pc -> trigger();
    p -> buffs.tot_trigger -> trigger();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_melee_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "vanish", p, p -> find_class_spell( "Vanish" ), options_str )
  {
    harmful = false;
  }

  virtual bool ready()
  {
    rogue_t* p = cast();

    if ( p -> buffs.stealthed -> check() )
      return false;

    return rogue_melee_attack_t::ready();
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_melee_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_melee_attack_t( "vendetta", p, p -> find_class_spell( "Vendetta" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }
  
  void execute()
  {
    rogue_melee_attack_t::execute();

    rogue_td_t* td = cast_td( execute_state -> target );

    td -> debuffs_vendetta -> trigger();
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  struct deadly_poison_dd_t : public rogue_poison_t
  {
    deadly_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_instant", p, p -> find_spell( 113780 ) )
    {
      direct_power_mod = data().extra_coeff();
      harmful          = true;
    }
  };
  
  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_dot", p, p -> find_class_spell( "Deadly Poison" ) -> effectN( 1 ).trigger() )
    {
      harmful        = true;
      tick_may_crit  = true;
      tick_power_mod = data().extra_coeff();
      dot_behavior   = DOT_REFRESH;
    }
  };
  
  deadly_poison_dd_t*  proc_instant;
  deadly_poison_dot_t* proc_dot;
  double               proc_chance;
  
  deadly_poison_t( rogue_t* player ) : 
    rogue_poison_t( "deadly_poison", player, player -> find_class_spell( "Deadly Poison" ) ),
    proc_instant( 0 ), proc_dot( 0 ), proc_chance( 0 )
  {
    quiet          = true;
    may_miss       = false;
    proc_chance    = player -> find_class_spell( "Deadly Poison" ) -> proc_chance();
    proc_chance   += player -> spec.improved_poisons -> effectN( 1 ).percent();

    proc_instant = new deadly_poison_dd_t( player );
    //add_child( proc_instant );
    proc_dot     = new deadly_poison_dot_t( player );
    //add_child( proc_dot );
  }
  
  virtual void impact( action_state_t* state )
  {
    bool is_up = cast_td( state -> target ) -> dots_deadly_poison -> ticking;

    rogue_poison_t::impact( state );

    if ( ! p() -> sim -> overrides.magic_vulnerability && p() -> spell.master_poisoner -> ok() )
      state -> target -> debuffs.magic_vulnerability -> trigger( 1, -1.0, -1.0, p() -> spell.master_poisoner -> duration() );

    double chance = proc_chance;
    if ( p() -> buffs.envenom -> up() )
      chance += p() -> buffs.envenom -> data().effectN( 2 ).percent();

    if ( p() -> rng.deadly_poison -> roll( chance ) )
    {
      proc_dot -> execute();
      if ( is_up )
        proc_instant -> execute();
    }
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  wound_poison_t( rogue_t* player ) : 
    rogue_poison_t( "wound_poison", player, player -> find_class_spell( "Wound Poison" ) -> effectN( 1 ).trigger() )
  {
    direct_power_mod = data().extra_coeff();
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      player_t* target;
      expiration_t( sim_t* sim, rogue_t* p, player_t* t ) : event_t( sim, p ), target( t )
      {
        name = "Wound Poison Expiration";
        sim -> add_event( this, timespan_t::from_seconds( 15.0 ) );
      }

      virtual void execute()
      {
        rogue_t* p = debug_cast< rogue_t* >( player );
        p -> expirations_.wound_poison = 0;
      }
    };

    rogue_t* p = cast();
    double chance;

    if ( p -> buffs.deadly_proc -> check() )
    {
      chance = 1.0;
      may_crit = true;
    }
    else if ( p -> buffs.shiv -> check() )
    {
      chance = 1.0;
      may_crit = false;
    }
    else
    {
      double PPM = 21.43;
      chance = weapon -> proc_chance_on_swing( PPM );
      may_crit = true;
    }

    if ( p -> rng.wound_poison -> roll( chance ) )
    {
      rogue_poison_t::execute();

      if ( result_is_hit( execute_state -> result ) )
      {
        event_t*& e = p -> expirations_.wound_poison;

        if ( e )
          e -> reschedule( timespan_t::from_seconds( 15.0 ) );
        else
          e = new ( sim ) expiration_t( sim, p, target );
      }
    }
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  int lethal_poison;
  int nonlethal_poison;
  bool executed;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    lethal_poison( 0 ), nonlethal_poison( 0 ),
    executed( false )
  {
    std::string lethal_str;
    std::string nonlethal_str;

    option_t options[] =
    {
      { "lethal",    OPT_STRING,  &lethal_str },
      { "nonlethal", OPT_STRING,  &nonlethal_str },
      { NULL,        OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( p -> main_hand_weapon.type != WEAPON_NONE || p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( lethal_str == "deadly"    ) lethal_poison = DEADLY_POISON;
      if ( lethal_str == "wound"     ) lethal_poison = WOUND_POISON;
    }
    
    if ( ! p -> active_lethal_poison )
    {
      if ( lethal_poison == DEADLY_POISON ) p -> active_lethal_poison = new deadly_poison_t( p );
      if ( lethal_poison == WOUND_POISON  ) p -> active_lethal_poison = new wound_poison_t( p );
    }
  }
  
  virtual void execute()
  {
    executed = true;
  }

  virtual bool ready()
  {
    return ! executed;
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( rogue_t* p, const std::string& options_str ) :
    spell_t( "stealth", p, p -> find_class_spell( "Stealth" ) ), used( false )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    if ( sim -> log )
      sim -> output( "%s performs %s", p -> name(), name() );

    p -> buffs.stealthed -> trigger();
    used = true;
  }

  virtual bool ready()
  {
    return ! used;
  }

  virtual void reset()
  {
    spell_t::reset();
    used = false;
  }
};

// ==========================================================================
// Buffs
// ==========================================================================

struct fof_fod_buff_t : public buff_t
{
  fof_fod_buff_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "legendary_daggers" ).duration( timespan_t::from_seconds( 6.0 ) ).cd( timespan_t::zero() ) )
  { }

  virtual void expire()
  {
    buff_t::expire();

    rogue_t* p = debug_cast< rogue_t* >( player );
    p -> buffs.fof_p3 -> expire();
  }
};

struct bandits_guile_t : public buff_t
{
  bandits_guile_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "bandits_guile" )
            .quiet( true )
            .max_stack( 12 )
            .duration( p -> find_spell( 84745 ) -> duration() )
            .chance( p -> find_specialization_spell( "Bandit's Guile" ) -> proc_chance() ) )
  { }

  void execute( int stacks = 1, double value = -1.0, timespan_t duration = timespan_t::min() )
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    buff_t::execute( stacks, value, duration );

    if ( current_stack == 4 )
      p -> buffs.shallow_insight -> trigger();
    else if ( current_stack == 8 )
    {
      p -> buffs.shallow_insight -> expire();
      p -> buffs.moderate_insight -> trigger();
    }
    else if ( current_stack == 12 )
    {
      if ( p -> buffs.deep_insight -> check() )
        return;
      
      p -> buffs.moderate_insight -> expire();
      p -> buffs.deep_insight -> trigger();
    }
  }
};

} // UNNAMED NAMESPACE

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

rogue_td_t::rogue_td_t( player_t* target, rogue_t* source ) :
    actor_pair_t( target, source )
{
  combo_points = new combo_points_t( target );
  
  dots_deadly_poison = target -> get_dot( "deadly_poison_dot", source );
  
  debuffs_anticipation_charges = buff_creator_t( *this, "anticipation_charges", source -> find_talent_spell( "Anticipation" ) )
                                 .max_stack( 5 );

  const spell_data_t* fw = source -> find_specialization_spell( "Find Weakness" );
  const spell_data_t* fwt = fw -> effectN( 1 ).trigger();
  debuffs_find_weakness = buff_creator_t( *this, "find_weakness", fwt )
                          .default_value( fw -> effectN( 1 ).percent() )
                          .chance( fw -> proc_chance() );

  const spell_data_t* vd = source -> find_specialization_spell( "Vendetta" );
  debuffs_vendetta =           buff_creator_t( *this, "vendetta", vd )
                               .default_value( vd -> effectN( 1 ).percent() );
  
  const spell_data_t* rs = source -> find_specialization_spell( "Revealing Strike" );
  debuffs_revealing_strike = buff_creator_t( *this, "revealing_strike", rs )
                             .default_value( rs -> effectN( 3 ).percent() )
                             .chance( rs -> ok() );
    
}

// rogue_t::composite_attribute_multiplier ==================================

double rogue_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_AGILITY )
    m *= 1.0 + spec.sinister_calling -> effectN( 1 ).percent();

  return m;
}

// rogue_t::composite_attack_speed ==========================================

double rogue_t::composite_attack_speed()
{
  double h = player_t::composite_attack_speed();

  if ( buffs.slice_and_dice -> check() )
    h *= 1.0 / ( 1.0 + buffs.slice_and_dice -> value() );

  if ( buffs.adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush -> value() );

  return h;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_attack_power_multiplier ===============================

double rogue_t::composite_attack_power_multiplier()
{
  double m = player_t::composite_attack_power_multiplier();

  m *= 1.0 + spec.vitality -> effectN( 2 ).percent();

  return m;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( buffs.master_of_subtlety -> check() ||
       ( spec.master_of_subtlety -> ok() && ( buffs.stealthed -> check() || buffs.vanish -> check() ) ) )
    m *= 1.0 + buffs.master_of_subtlety -> value();

  m *= 1.0 + buffs.shallow_insight -> value();

  m *= 1.0 + buffs.moderate_insight -> value();

  m *= 1.0 + buffs.deep_insight -> value();

  return m;
}

// rogue_t::init_actions ====================================================

void rogue_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    if ( level >= 80 )
    {
      // Flask
      precombat_list += "/flask,type=";
      precombat_list += ( level > 85 ) ? "spring_blossoms" : "winds";

      // Food
      precombat_list += "/food,type=";
      precombat_list += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
    }

    precombat_list += "/apply_poison,lethal=deadly";
    precombat_list += "/snapshot_stats";

    // Prepotion
    precombat_list += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";

    // Potion use
    precombat_list += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";

    action_list_str += "/auto_attack";

    if ( spec.master_of_subtlety -> ok() )
      action_list_str += "/stealth";

    action_list_str += "/kick";

    if ( specialization() == ROGUE_ASSASSINATION )
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_profession_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier13_2pc_melee";

      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.down";
      action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
      action_list_str += "/slice_and_dice,if=combo_points>=3&buff.slice_and_dice.remains<2";

      action_list_str += "/rupture,if=(!ticking|ticks_remain<2)&time<6";
      action_list_str += "/vendetta";

      action_list_str += "/rupture,if=(!ticking|ticks_remain<2)&buff.slice_and_dice.remains>6";

      action_list_str += "/envenom,if=combo_points>=4&buff.envenom.down";
      action_list_str += "/envenom,if=combo_points>=4&energy>90";
      action_list_str += "/envenom,if=combo_points>=2&buff.slice_and_dice.remains<3";
      action_list_str += "/dispatch,if=combo_points<5&(target.health.pct<35|buff.blindside.up)";
      action_list_str += "/mutilate,if=position_front&combo_points<5&target.health.pct<35";
      action_list_str += "/mutilate,if=combo_points<4&target.health.pct>=35";
    }
    else if ( specialization() == ROGUE_COMBAT )
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_profession_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier13_2pc_melee";

      // TODO: Add Blade Flurry
      action_list_str += "/slice_and_dice,if=buffs.slice_and_dice.down";
      action_list_str += "/slice_and_dice,if=buffs.slice_and_dice.remains<2";
      
      action_list_str += "/killing_spree,if=energy<35&buffs.slice_and_dice.remains>4&buffs.adrenaline_rush.down";

      action_list_str += "/adrenaline_rush,if=energy<35";

      action_list_str += "/eviscerate,if=combo_points=5&buffs.bandits_guile.stack>=12";

      action_list_str += "/rupture,if=!ticking&combo_points=5&target.time_to_die>10";
      action_list_str += "/eviscerate,if=combo_points=5";

      action_list_str += "/revealing_strike,if=combo_points=4&buffs.revealing_strike.down";

      action_list_str += "/sinister_strike,if=combo_points<5";
    }
    else if ( specialization() == ROGUE_SUBTLETY )
    {
      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier13_2pc_melee";

      action_list_str += "/pool_energy,for_next=1";
      action_list_str += "/shadow_dance,if=energy>85&combo_points<5&buffs.stealthed.down";

      int num_items = ( int ) items.size();
      int hand_enchant_found = -1;
      int found_item = -1;

      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          if ( items[ i ].slot == SLOT_HANDS )
          {
            hand_enchant_found = i;
            continue;
          }
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
          if ( found_item < 0 )
          {
            action_list_str += ",if=buffs.shadow_dance.up";
            found_item = i;
          }
          else
          {
            action_list_str += ",if=buffs.shadow_dance.cooldown_remains>20";
          }
        }
      }
      if ( hand_enchant_found >= 0 )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ hand_enchant_found ].name();
        if ( found_item < 0 )
        {
          action_list_str += ",if=buffs.shadow_dance.up";
        }
        else
        {
          action_list_str += ",if=buffs.shadow_dance.cooldown_remains>20";
        }
      }

      action_list_str += init_use_profession_actions( ( found_item >= 0 ) ? "" : ",if=buffs.shadow_dance.up|position_front" );

      action_list_str += init_use_racial_actions( ",if=buffs.shadow_dance.up" );

      action_list_str += "/pool_energy,for_next=1";
      action_list_str += "/vanish,if=time>10&energy>60&combo_points<=1&cooldowns.shadowstep.remains<=0&!buffs.shadow_dance.up&!buffs.master_of_subtlety.up&!target.debuff.find_weakness.up";

      action_list_str += "/shadowstep,if=buffs.stealthed.up|buffs.shadow_dance.up";

      action_list_str += "/premeditation,if=(combo_points<=3&cooldowns.honor_among_thieves.remains>1.75)|combo_points<=2";

      action_list_str += "/ambush,if=combo_points<=4";

      action_list_str += "/preparation,if=cooldowns.vanish.remains>60";

      action_list_str += "/slice_and_dice,if=buffs.slice_and_dice.remains<3&combo_points=5";

      action_list_str += "/rupture,if=combo_points=5&!ticking";

      action_list_str += "/recuperate,if=combo_points=5&remains<3";

      action_list_str += "/eviscerate,if=combo_points=5&dot.rupture.remains>1";

      action_list_str += "/hemorrhage,if=combo_points<4&(dot.hemorrhage.remains<4|position_front)";
      action_list_str += "/hemorrhage,if=combo_points<5&energy>80&(dot.hemorrhage.remains<4|position_front)";

      action_list_str += "/backstab,if=combo_points<4";
      action_list_str += "/backstab,if=combo_points<5&energy>80";
    }
    else
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */

      action_list_str += "/pool_energy,if=energy<60&buffs.slice_and_dice.remains<5";
      action_list_str += "/slice_and_dice,if=combo_points>=3&buffs.slice_and_dice.remains<2";
      action_list_str += "/sinister_strike,if=combo_points<5";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack"         ) return new auto_melee_attack_t  ( this, options_str );
  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "dispatch"            ) return new dispatch_t           ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "pool_energy"         ) return new pool_energy_t        ( this, options_str );
  if ( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if ( name == "preparation"         ) return new preparation_t        ( this, options_str );
  if ( name == "recuperate"          ) return new recuperate_t         ( this, options_str );
  if ( name == "revealing_strike"    ) return new revealing_strike_t   ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );
  if ( name == "tricks_of_the_trade" ) return new tricks_of_the_trade_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "combo_points" )
  {
    struct combo_points_expr_t : public expr_t
    {
      const action_t& action;
      combo_points_expr_t( action_t* a ) : expr_t( "combo_points" ),
        action( *a )
      {}

      virtual double evaluate()
      {
        rogue_t* p = debug_cast<rogue_t*>( action.player );
        rogue_td_t* td = p -> get_target_data( action.target );
        return td -> combo_points -> count;
      }
    };
    return new combo_points_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base()
{
  player_t::init_base();

  base.attack_power = ( level * 2 );
  initial.attack_power_per_strength = 1.0;
  initial.attack_power_per_agility  = 2.0;

  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER )
    resources.base[ RESOURCE_ENERGY ] = 100 + spec.assassins_resolve -> effectN( 1 ).base_value();

  base_energy_regen_per_second = 10 + spec.vitality -> effectN( 1 ).base_value();

  base_gcd = timespan_t::from_seconds( 1.0 );

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;
}

// rogue_t::init_talents ====================================================

void rogue_t::init_talents()
{
  // Talents
#if 0
  // Assasination
  talents.cold_blood                 = find_talent( "Cold Blood" );
  talents.coup_de_grace              = find_talent( "Coup de Grace" );
  talents.cut_to_the_chase           = find_talent( "Cut to the Chase" );
  talents.improved_expose_armor      = find_talent( "Improved Expose Armor" );
  talents.lethality                  = find_talent( "Lethality" );
  talents.master_poisoner            = find_talent( "Master Poisoner" );
  talents.murderous_intent           = find_talent( "Murderous Intent" );
  talents.overkill                   = find_talent( "Overkill" );
  talents.puncturing_wounds          = find_talent( "Puncturing Wounds" );
  talents.quickening                 = find_talent( "Quickening" );
  talents.ruthlessness               = find_talent( "Ruthlessness" );
  talents.seal_fate                  = find_talent( "Seal Fate" );
  talents.vendetta                   = find_talent( "Vendetta" );
  talents.venomous_wounds            = find_talent( "Venomous Wounds" );
  talents.vile_poisons               = find_talent( "Vile Poisons" );

  // Combat
  talents.adrenaline_rush            = find_talent( "Adrenaline Rush" );
  talents.aggression                 = find_talent( "Aggression" );
  talents.bandits_guile              = find_talent( "Bandit's Guile" );
  talents.combat_potency             = find_talent( "Combat Potency" );
  talents.improved_sinister_strike   = find_talent( "Improved Sinister Strike" );
  talents.improved_slice_and_dice    = find_talent( "Improved Slice and Dice" );
  talents.killing_spree              = find_talent( "Killing Spree" );
  talents.lightning_reflexes         = find_talent( "Lightning Reflexes" );
  talents.precision                  = find_talent( "Precision" );
  talents.restless_blades            = find_talent( "Restless Blades" );
  talents.revealing_strike           = find_talent( "Revealing Strike" );
  talents.savage_combat              = find_talent( "Savage Combat" );

  // Subtlety
  talents.elusiveness                = find_talent( "Elusiveness" );
  talents.energetic_recovery         = find_talent( "Energetic Recovery" );
  talents.find_weakness              = find_talent( "Find Weakness" );
  talents.hemorrhage                 = find_talent( "Hemorrhage" );
  talents.honor_among_thieves        = find_talent( "Honor Among Thieves" );
  talents.improved_ambush            = find_talent( "Improved Ambush" );
  talents.initiative                 = find_talent( "Initiative" );
  talents.opportunity                = find_talent( "Opportunity" );
  talents.premeditation              = find_talent( "Premeditation" );
  talents.preparation                = find_talent( "Preparation" );
  talents.relentless_strikes         = find_talent( "Relentless Strikes" );
  talents.sanguinary_vein            = find_talent( "Sanguinary Vein" );
  talents.serrated_blades            = find_talent( "Serrated Blades" );
  talents.shadow_dance               = find_talent( "Shadow Dance" );
  talents.slaughter_from_the_shadows = find_talent( "Slaughter from the Shadows" );
  talents.waylay                     = find_talent( "Waylay" );
#endif
  player_t::init_talents();
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Assassination
  spec.assassins_resolve    = find_specialization_spell( "Assassin's Resolve" );
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );
  spec.cut_to_the_chase     = find_specialization_spell( "Cut to the Chase" );
  spec.blindside            = find_specialization_spell( "Blindside" );

  // Combat
  spec.ambidexterity        = find_specialization_spell( "Ambidexterity" );
  spec.vitality             = find_specialization_spell( "Vitality" );
  spec.combat_potency       = find_specialization_spell( "Combat Potency" );
  spec.restless_blades      = find_specialization_spell( "Restless Blades" );
  spec.bandits_guile        = find_specialization_spell( "Bandit's Guile" );
  spec.killing_spree        = find_specialization_spell( "Killing Spree" );
    
  // Subtlety
  spec.master_of_subtlety   = find_specialization_spell( "Master of Subtlety" );
  spec.sinister_calling     = find_specialization_spell( "Sinister Calling" );
  spec.find_weakness        = find_specialization_spell( "Find Weakness" );
  spec.honor_among_thieves  = find_specialization_spell( "Honor Among Thieves" );
  spec.sanguinary_vein      = find_specialization_spell( "Sanguinary Vein" );
  spec.energetic_recovery   = find_specialization_spell( "Energetic Recovery" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );

  // Masteries
  mastery.potent_poisons    = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_COMBAT );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.master_poisoner     = find_spell( 93068 );
  spell.relentless_strikes  = find_spell( 58423 );
  spell.tier13_2pc          = find_spell( 105864 );
  spell.tier13_4pc          = find_spell( 105865 );
  spell.bandits_guile_value = find_spell( 84747 );
  
  // Glyphs
  glyph.kick                = find_glyph_spell( "Glyph of Kick" );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {     0,     0, 105849, 105865,     0,     0,     0,     0 }, // Tier13
    {     0,     0, 123116, 123122,     0,     0,     0,     0 }, // Tier14
    {     0,     0,      0,      0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush    = get_gain( "adrenaline_rush"    );
  gains.combat_potency     = get_gain( "combat_potency"     );
  gains.energy_refund      = get_gain( "energy_refund"      );
  gains.murderous_intent   = get_gain( "murderous_intent"   );
  gains.overkill           = get_gain( "overkill"           );
  gains.recuperate         = get_gain( "recuperate"         );
  gains.relentless_strikes = get_gain( "relentless_strikes" );
  gains.venomous_wounds    = get_gain( "venomous_wounds"    );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.deadly_poison            = get_proc( "deadly_poisons"        );
  procs.honor_among_thieves      = get_proc( "honor_among_thieves"   );
  procs.main_gauche              = get_proc( "main_gauche"           );
  procs.seal_fate                = get_proc( "seal_fate"             );
  procs.venomous_wounds          = get_proc( "venomous_wounds"       );
}

// rogue_t::init_uptimes ====================================================

void rogue_t::init_benefits()
{
  player_t::init_benefits();

  benefits.poisoned   = get_benefit( "poisoned"   );
}

// rogue_t::init_rng ========================================================

void rogue_t::init_rng()
{
  player_t::init_rng();

  rng.deadly_poison         = get_rng( "deadly_poison"         );
  rng.honor_among_thieves   = get_rng( "honor_among_thieves"   );
  rng.main_gauche           = get_rng( "main_gauche"           );
  rng.relentless_strikes    = get_rng( "relentless_strikes"    );
  rng.revealing_strike      = get_rng( "revealing_strike"      );
  rng.venomous_wounds       = get_rng( "venomous_wounds"       );
  rng.wound_poison          = get_rng( "wound_poison"          );
  rng.hat_interval          = get_rng( "hat_interval" );
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
  scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
  scales_with[ STAT_HIT_RATING2           ] = true;
}

// rogue_t::init_buffs ======================================================

void rogue_t::init_buffs()
{
  // Handle the Legendary here, as it's called after init_items()
  if ( find_item( "vengeance" ) && find_item( "fear" ) )
  {
    fof_p1 = 1;
  }
  else if ( find_item( "the_sleeper" ) && find_item( "the_dreamer" ) )
  {
    fof_p2 = 1;
  }
  else if ( find_item( "golad_twilight_of_aspects" ) && find_item( "tiriosh_nightmare_of_ages" ) )
  {
    fof_p3 = 1;
  }

  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  
  buffs.bandits_guile       = new bandits_guile_t( this );
  buffs.adrenaline_rush     = buff_creator_t( this, "adrenaline_rush", find_class_spell( "Adrenaline Rush" ) )
                              .default_value( find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() );
  buffs.blindside           = buff_creator_t( this, "blindside", spec.blindside -> effectN( 1 ).trigger() )
                              .chance( spec.blindside -> proc_chance() );
  buffs.master_of_subtlety  = buff_creator_t( this, "master_of_subtlety", spec.master_of_subtlety )
                             .duration( timespan_t::from_seconds( 6.0 ) )
                             .default_value( spec.master_of_subtlety -> effectN( 1 ).percent() )
                             .chance( spec.master_of_subtlety -> ok() );
  // Killing spree buff has only 2 sec duration, main spell has 3, check.
  buffs.killing_spree       = buff_creator_t( this, "killing_spree", find_spell( 61851 ) )
                              .default_value( find_spell( 61851 ) -> effectN( 3 ).percent() )
                              .duration( spec.killing_spree -> duration() )
                              .chance( spec.killing_spree -> ok() );
  buffs.shadow_dance       = buff_creator_t( this, "shadow_dance", find_specialization_spell( "Shadow Dance" ) );
  buffs.deadly_proc        = buff_creator_t( this, "deadly_proc");
  buffs.shallow_insight    = buff_creator_t( this, "shallow_insight", find_spell( 84745 ) );
  buffs.moderate_insight   = buff_creator_t( this, "moderate_insight", find_spell( 84746 ) );
  buffs.deep_insight       = buff_creator_t( this, "deep_insight", find_spell( 84747 ) );
  buffs.recuperate         = buff_creator_t( this, "recuperate" );
  buffs.shiv               = buff_creator_t( this, "shiv" );
  buffs.stealthed          = buff_creator_t( this, "stealthed" );
  buffs.tier13_2pc         = buff_creator_t( this, "tier13_2pc", spell.tier13_2pc )
                             .chance( set_bonus.tier13_2pc_melee() ? 1.0 : 0 );
  buffs.tot_trigger        = buff_creator_t( this, "tricks_of_the_trade_trigger", find_spell( 57934 ) )
                             .activated( true );
  buffs.vanish             = buff_creator_t( this, "vanish" )
                             .duration( timespan_t::from_seconds( 3.0 ) );

  buffs.envenom            = buff_creator_t( this, "envenom", find_specialization_spell( "Envenom" ) );
  buffs.slice_and_dice     = buff_creator_t( this, "slice_and_dice", find_class_spell( "Slice and Dice" ) );

  // Legendary buffs
  buffs.fof_p1            = stat_buff_creator_t( this, "suffering", find_spell( 109959 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109959 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p1 );
  buffs.fof_p2            = stat_buff_creator_t( this, "nightmare", find_spell( 109955 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109955 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p2 );
  buffs.fof_p3            = stat_buff_creator_t( this, "shadows_of_the_destroyer", find_spell( 109939 ) -> effectN( 1 ).trigger() )
                            .add_stat( STAT_AGILITY, find_spell( 109939 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() )
                            .chance( fof_p3 );
  buffs.fof_fod           = new fof_fod_buff_t( this );
}

// trigger_honor_among_thieves ==============================================

struct honor_among_thieves_callback_t : public action_callback_t
{
  honor_among_thieves_callback_t( rogue_t* r ) : action_callback_t( r ) {}

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    rogue_t* rogue = debug_cast< rogue_t* >( listener );

    if ( a )
    {
      // only procs from specials; repeating is here for hunter autoshot, which doesn't proc it either
      if ( ! a -> special || a -> repeating )
        return;

      // doesn't proc from pets (only tested for hunter pets though)
      if ( a -> player -> is_pet() )
        return;

      a -> player -> procs.hat_donor -> occur();
    }

//    if ( sim -> debug )
//      sim -> output( "Eligible For Honor Among Thieves" );

    if ( rogue -> cooldowns.honor_among_thieves -> remains() > timespan_t::zero() )
      return;

    if ( ! rogue -> rng.honor_among_thieves -> roll( rogue -> spec.honor_among_thieves -> proc_chance() ) )
      return;

    rogue_td_t* td = rogue -> get_target_data( rogue -> target );

    td -> combo_points -> add( 1, rogue -> spec.honor_among_thieves -> name_cstr() );

    rogue -> procs.honor_among_thieves -> occur();

    rogue -> cooldowns.honor_among_thieves -> start( timespan_t::from_seconds( 2.0 ) );
  }
};

// rogue_t::register_callbacks ==============================================

void rogue_t::register_callbacks()
{
  player_t::register_callbacks();

  if ( spec.honor_among_thieves -> ok() )
  {
    action_callback_t* cb = new honor_among_thieves_callback_t( this );

    callbacks.register_attack_callback( RESULT_CRIT_MASK, cb );
    callbacks.register_spell_callback ( RESULT_CRIT_MASK, cb );
    callbacks.register_tick_callback( RESULT_CRIT_MASK, cb );

    if ( virtual_hat_interval < timespan_t::zero() )
    {
      virtual_hat_interval = timespan_t::from_seconds( 2.20 );
    }
    if ( virtual_hat_interval > timespan_t::zero() )
    {
      virtual_hat_callback = cb;
    }
    else
    {
      for ( size_t i = 0; i < sim -> player_list.size(); i++ )
      {
        player_t* p = sim -> player_list[ i ];

        if ( p == this     ) continue;
        if ( p -> is_pet() ) continue;

        p -> callbacks.register_attack_callback( RESULT_CRIT_MASK, cb );
        p -> callbacks.register_spell_callback ( RESULT_CRIT_MASK, cb );
        p -> callbacks.register_tick_callback( RESULT_CRIT_MASK, cb );
      }
    }
  }
}

// rogue_t::combat_begin ====================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();

  if ( spec.honor_among_thieves -> ok() )
  {
    if ( virtual_hat_interval > timespan_t::zero() )
    {
      struct virtual_hat_event_t : public event_t
      {
        action_callback_t* callback;
        timespan_t interval;

        virtual_hat_event_t( sim_t* sim, rogue_t* p, action_callback_t* cb, timespan_t i ) :
          event_t( sim, p ), callback( cb ), interval( i )
        {
          name = "Virtual HAT Event";
          timespan_t cooldown = timespan_t::from_seconds( 2.0 );
          timespan_t remainder = interval - cooldown;
          if ( remainder < timespan_t::zero() ) remainder = timespan_t::zero();
          timespan_t time = cooldown + p -> rng.hat_interval -> range( remainder*0.5, remainder*1.5 ) + timespan_t::from_seconds( 0.01 );
          sim -> add_event( this, time );
        }
        virtual void execute()
        {
          rogue_t* p = debug_cast<rogue_t*>( player );
          callback -> trigger( NULL );
          new ( sim ) virtual_hat_event_t( sim, p, callback, interval );
        }
      };
      new ( sim ) virtual_hat_event_t( sim, this, virtual_hat_callback, virtual_hat_interval );
    }
  }
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  for ( size_t i=0; i < sim -> actor_list.size(); i++ )
  {
    rogue_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }

  expirations_.reset();
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  if ( ! sim -> overrides.attack_haste && dbc.spell( 113742 ) -> is_level( level ) ) sim -> auras.attack_haste -> trigger();
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second()
{
  double r = player_t::energy_regen_per_second();

  return r;
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  // XXX review how this all stacks (additive/multiplicative)

  player_t::regen( periodicity );

  if ( buffs.adrenaline_rush -> up() )
  {
    if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second();

      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
    }
  }
  
  if ( buffs.slice_and_dice -> up() )
  {
    double rps = spec.energetic_recovery -> effectN( 1 ).base_value() /
                 buffs.slice_and_dice -> data().effectN( 2 ).period().total_seconds();

    resource_gain( RESOURCE_ENERGY, rps * periodicity.total_seconds(), gains.energetic_recovery );
  }
}

// rogue_t::available =======================================================

timespan_t rogue_t::available()
{
  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 )
    return timespan_t::from_seconds( 0.1 );

  return std::max(
    timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
    timespan_t::from_seconds( 0.1 )
  );
}

// rogue_t::copy_options ====================================================

void rogue_t::create_options()
{
  player_t::create_options();

  option_t rogue_options[] =
  {
    { "virtual_hat_interval",       OPT_TIMESPAN, &( virtual_hat_interval         ) },
    { "tricks_of_the_trade_target", OPT_STRING, &( tricks_of_the_trade_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, rogue_options );
}

// rogue_t::create_profile ==================================================

bool rogue_t::create_profile( std::string& profile_str, save_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL || stype == SAVE_ACTIONS )
  {
    if ( spec.honor_among_thieves -> ok() )
    {
      profile_str += "# These values represent the avg HAT donor interval of the raid.\n";
      profile_str += "# A negative value will make the Rogue use a programmed default interval.\n";
      profile_str += "# A zero value will disable virtual HAT procs and assume a real raid is being simulated.\n";
      profile_str += "virtual_hat_interval=-1\n";  // Force it to generate profiles using programmed default.
    }
  }

  return true;
}

// rogue_t::copy_from =======================================================

void rogue_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  rogue_t* p = dynamic_cast<rogue_t*>( source );
  assert( p );

  virtual_hat_interval = p -> virtual_hat_interval;
}

// rogue_t::decode_set ======================================================

int rogue_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "blackfang_battleweave" ) ) return SET_T13_MELEE;

  if ( strstr( s, "thousandfold_blades"   ) ) return SET_T14_MELEE;

  if ( strstr( s, "_gladiators_leather_" ) )  return SET_PVP_CASTER;

  return SET_NONE;
}

#endif // SC_ROGUE

// ROGUE MODULE INTERFACE ================================================

struct rogue_module_t : public module_t
{
  rogue_module_t() : module_t( ROGUE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE )
  {
    return new rogue_t( sim, name, r );
  }
  virtual bool valid() { return true; }
  virtual void init( sim_t* sim )
  {
    sim -> auras.honor_among_thieves = buff_creator_t( sim, "honor_among_thieves" );

    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.tricks_of_the_trade  = buff_creator_t( p, "tricks_of_the_trade" ).max_stack( 1 ).duration( timespan_t::from_seconds( 6.0 ) );
    }
  }
  virtual void combat_begin( sim_t* sim )
  {
    if ( sim -> overrides.honor_among_thieves )
    {
      sim -> auras.honor_among_thieves -> override();
    }
  }
  virtual void combat_end( sim_t* ) {}
};

} // UNNAMED NAMESPACE

module_t* module_t::rogue()
{
  static module_t* m = 0;
  if ( ! m ) m = new rogue_module_t();
  return m;
}
