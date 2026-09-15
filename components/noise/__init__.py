import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import media_player, number, select, speaker
from esphome.const import (
    CONF_CHANNELS,
    CONF_DURATION,
    CONF_ID,
    CONF_LEVEL,
    CONF_MEDIA_PLAYER,
    CONF_SAMPLE_RATE,
    CONF_SPEAKER,
    CONF_TRIGGER_ID,
    CONF_VARIANT,
    CONF_VOLUME,
)

AUTO_LOAD = ["select", "media_player", "number"]
MULTI_CONF = True

noise_ns = cg.esphome_ns.namespace("noise")
NoiseComponent = noise_ns.class_("NoiseComponent", cg.Component)
NoiseSelect = noise_ns.class_("NoiseSelect", select.Select)
NoiseMediaPlayer = noise_ns.class_("NoiseMediaPlayer", media_player.MediaPlayer)
NoiseVolumeNumber = noise_ns.class_("NoiseVolumeNumber", number.Number)
NoiseToneNumber = noise_ns.class_("NoiseToneNumber", number.Number)
NoiseSleepTimerNumber = noise_ns.class_("NoiseSleepTimerNumber", number.Number)

NoiseStartAction = noise_ns.class_("NoiseStartAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseStopAction = noise_ns.class_("NoiseStopAction", automation.Action, cg.Parented.template(NoiseComponent))
NoisePauseAction = noise_ns.class_("NoisePauseAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseResumeAction = noise_ns.class_("NoiseResumeAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseDuckAction = noise_ns.class_("NoiseDuckAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseUnduckAction = noise_ns.class_("NoiseUnduckAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseSetVolumeAction = noise_ns.class_("NoiseSetVolumeAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseSetToneAction = noise_ns.class_("NoiseSetToneAction", automation.Action, cg.Parented.template(NoiseComponent))

VARIANTS = [
    "white",
    "pink",
    "brown",
    "gray",
    "waves",
    "wind",
    "stream",
    "fan",
    "rain",
    "campfire",
    "heartbeat",
    "beep",
]

CONF_AIRPLAY_RECEIVER = "airplay_receiver"
CONF_DUCK_ON_MEDIA_PLAYERS = "duck_on_media_players"
CONF_PAUSE_ON_MEDIA_PLAYERS = "pause_on_media_players"
CONF_DUCK_LEVEL = "duck_level"

CONF_SELECT = "select"
CONF_TONE = "tone"
CONF_SLEEP_TIMER = "sleep_timer"
CONF_INITIAL_VOLUME = "initial_volume"
CONF_FADE_IN_TIME = "fade_in_time"
CONF_FADE_OUT_TIME = "fade_out_time"
CONF_ON_PLAY = "on_play"
CONF_ON_STOP = "on_stop"
CONF_ON_VARIANT_CHANGED = "on_variant_changed"

_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        conf_key=CONF_ON_PLAY,
        callback_method="add_on_play_callback",
        args=[],
    ),
    automation.CallbackAutomation(
        conf_key=CONF_ON_STOP,
        callback_method="add_on_stop_callback",
        args=[],
    ),
    automation.CallbackAutomation(
        conf_key=CONF_ON_VARIANT_CHANGED,
        callback_method="add_on_variant_changed_callback",
        args=[(cg.std_string, "variant")],
    ),
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(NoiseComponent),
            cv.Optional(CONF_SPEAKER): cv.use_id(speaker.Speaker),
            # Direct output to an AirPlay 2 receiver (esphome-airplay2)
            cv.Optional(CONF_AIRPLAY_RECEIVER): cv.use_id(media_player.MediaPlayer),
            # Multi-source audio: automatically duck noise when other media players play
            cv.Optional(CONF_DUCK_ON_MEDIA_PLAYERS): cv.ensure_list(cv.use_id(media_player.MediaPlayer)),
            # Multi-source audio: automatically pause noise when other media players play
            cv.Optional(CONF_PAUSE_ON_MEDIA_PLAYERS): cv.ensure_list(cv.use_id(media_player.MediaPlayer)),
            cv.Optional(CONF_DUCK_LEVEL, default="20%"): cv.percentage,
            # 0 (default) = follow speaker/airplay sample rate.
            cv.Optional(CONF_SAMPLE_RATE, default=0): cv.int_range(min=0, max=48000),
            # 0 (default) = follow speaker/airplay channels, 1 = mono, 2 = stereo.
            cv.Optional(CONF_CHANNELS, default=0): cv.int_range(min=0, max=2),
            cv.Optional(CONF_INITIAL_VOLUME, default="100%"): cv.percentage,
            cv.Optional(CONF_FADE_IN_TIME, default="100ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_FADE_OUT_TIME, default="100ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_SELECT): select.select_schema(
                NoiseSelect,
                entity_category=cv.UNDEFINED,
                icon="mdi:white-balance-sunny",
            ),
            cv.Optional(CONF_MEDIA_PLAYER): media_player.media_player_schema(
                NoiseMediaPlayer,
                icon="mdi:speaker-play",
            ),
            cv.Optional(CONF_VOLUME): number.number_schema(
                NoiseVolumeNumber,
                icon="mdi:volume-high",
                unit_of_measurement="%",
            ),
            cv.Optional(CONF_TONE): number.number_schema(
                NoiseToneNumber,
                icon="mdi:tune",
                unit_of_measurement="%",
            ),
            cv.Optional(CONF_SLEEP_TIMER): number.number_schema(
                NoiseSleepTimerNumber,
                icon="mdi:timer-outline",
                unit_of_measurement="min",
            ),
            cv.Optional(CONF_ON_PLAY): automation.validate_automation(automation.AUTOMATION_SCHEMA),
            cv.Optional(CONF_ON_STOP): automation.validate_automation(automation.AUTOMATION_SCHEMA),
            cv.Optional(CONF_ON_VARIANT_CHANGED): automation.validate_automation(automation.AUTOMATION_SCHEMA),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_SPEAKER, CONF_AIRPLAY_RECEIVER),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    cg.add(var.set_channels(config[CONF_CHANNELS]))
    cg.add(var.set_volume(config[CONF_INITIAL_VOLUME]))
    cg.add(var.set_fade_in_time(config[CONF_FADE_IN_TIME]))
    cg.add(var.set_fade_out_time(config[CONF_FADE_OUT_TIME]))
    cg.add(var.set_default_duck_level(config[CONF_DUCK_LEVEL]))

    if CONF_SPEAKER in config:
        speaker_var = await cg.get_variable(config[CONF_SPEAKER])
        cg.add(var.set_speaker(speaker_var))

    if CONF_AIRPLAY_RECEIVER in config:
        cg.add_define("USE_NOISE_AIRPLAY")
        ap_var = await cg.get_variable(config[CONF_AIRPLAY_RECEIVER])
        cg.add(var.set_airplay_receiver(ap_var))

    if CONF_DUCK_ON_MEDIA_PLAYERS in config:
        for player_id in config[CONF_DUCK_ON_MEDIA_PLAYERS]:
            p_var = await cg.get_variable(player_id)
            cg.add(var.add_duck_source(p_var))

    if CONF_PAUSE_ON_MEDIA_PLAYERS in config:
        for player_id in config[CONF_PAUSE_ON_MEDIA_PLAYERS]:
            p_var = await cg.get_variable(player_id)
            cg.add(var.add_pause_source(p_var))

    if CONF_SELECT in config:
        select_var = await select.new_select(
            config[CONF_SELECT],
            options=["Off"] + [v.capitalize() for v in VARIANTS],
        )
        cg.add(select_var.set_parent(var))
        cg.add(var.set_select(select_var))

    if CONF_MEDIA_PLAYER in config:
        mp_var = await media_player.new_media_player(config[CONF_MEDIA_PLAYER])
        cg.add(mp_var.set_parent(var))
        cg.add(var.set_media_player(mp_var))

    if CONF_VOLUME in config:
        vol_var = await number.new_number(
            config[CONF_VOLUME],
            min_value=0.0,
            max_value=100.0,
            step=1.0,
        )
        cg.add(vol_var.set_parent(var))
        cg.add(var.set_volume_number(vol_var))

    if CONF_TONE in config:
        tone_var = await number.new_number(
            config[CONF_TONE],
            min_value=0.0,
            max_value=100.0,
            step=1.0,
        )
        cg.add(tone_var.set_parent(var))
        cg.add(var.set_tone_number(tone_var))

    if CONF_SLEEP_TIMER in config:
        timer_var = await number.new_number(
            config[CONF_SLEEP_TIMER],
            min_value=0.0,
            max_value=180.0,
            step=5.0,
        )
        cg.add(timer_var.set_parent(var))
        cg.add(var.set_sleep_timer_number(timer_var))

    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)


@automation.register_action(
    "noise.start",
    NoiseStartAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Optional(CONF_VARIANT): cv.templatable(cv.one_of(*VARIANTS, lower=True)),
            cv.Optional(CONF_DURATION): cv.templatable(cv.positive_time_period_milliseconds),
            cv.Optional(CONF_VOLUME): cv.templatable(cv.percentage),
        }
    ),
    synchronous=True,
)
async def noise_start_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    if CONF_VARIANT in config:
        template_ = await cg.templatable(config[CONF_VARIANT], args, cg.std_string)
        cg.add(var.set_variant(template_))
    if CONF_DURATION in config:
        template_ = await cg.templatable(config[CONF_DURATION], args, cg.uint32)
        cg.add(var.set_duration(template_))
    if CONF_VOLUME in config:
        template_ = await cg.templatable(config[CONF_VOLUME], args, cg.float_)
        cg.add(var.set_volume(template_))
    return var


@automation.register_action(
    "noise.stop",
    NoiseStopAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
        }
    ),
    synchronous=True,
)
async def noise_stop_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "noise.pause",
    NoisePauseAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
        }
    ),
    synchronous=True,
)
async def noise_pause_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "noise.resume",
    NoiseResumeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
        }
    ),
    synchronous=True,
)
async def noise_resume_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "noise.duck",
    NoiseDuckAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Optional(CONF_LEVEL, default="20%"): cv.templatable(cv.percentage),
        }
    ),
    synchronous=True,
)
async def noise_duck_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_LEVEL], args, cg.float_)
    cg.add(var.set_level(template_))
    return var


@automation.register_action(
    "noise.unduck",
    NoiseUnduckAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
        }
    ),
    synchronous=True,
)
async def noise_unduck_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "noise.set_volume",
    NoiseSetVolumeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Required(CONF_VOLUME): cv.templatable(cv.percentage),
        }
    ),
    synchronous=True,
)
async def noise_set_volume_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_VOLUME], args, cg.float_)
    cg.add(var.set_volume(template_))
    return var


@automation.register_action(
    "noise.set_tone",
    NoiseSetToneAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Required(CONF_TONE): cv.templatable(cv.percentage),
        }
    ),
    synchronous=True,
)
async def noise_set_tone_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_TONE], args, cg.float_)
    cg.add(var.set_tone(template_))
    return var