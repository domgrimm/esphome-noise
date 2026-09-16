import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core
from esphome.components import media_player, number, select, sensor, speaker, web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.const import (
    CONF_CHANNELS,
    CONF_DURATION,
    CONF_ID,
    CONF_LEVEL,
    CONF_MAX_VALUE,
    CONF_MEDIA_PLAYER,
    CONF_MIN_VALUE,
    CONF_SAMPLE_RATE,
    CONF_SPEAKER,
    CONF_STEP,
    CONF_TRIGGER_ID,
    CONF_VARIANT,
    CONF_VOLUME,
    CONF_WEB_SERVER,
)

def AUTO_LOAD(config):
    configs = config if isinstance(config, list) else [config]
    loads = set()
    for conf in configs:
        if not isinstance(conf, dict):
            continue
        if CONF_SELECT in conf:
            loads.add("select")
        if (
            CONF_MEDIA_PLAYER in conf
            or CONF_AIRPLAY_RECEIVER in conf
            or CONF_DUCK_ON_MEDIA_PLAYERS in conf
            or CONF_PAUSE_ON_MEDIA_PLAYERS in conf
            or CONF_SPEAKER_MEDIA_PLAYER in conf
            or CONF_MASTER_MEDIA_PLAYER in conf
        ):
            loads.add("media_player")
        if (
            CONF_VOLUME in conf
            or CONF_TONE in conf
            or CONF_SLEEP_TIMER in conf
        ):
            loads.add("number")
        if CONF_TIME_LEFT in conf:
            loads.add("sensor")
        if CONF_WEB_SERVER in conf and conf[CONF_WEB_SERVER]:
            loads.add("web_server_base")
    return list(loads)

MULTI_CONF = True

noise_ns = cg.esphome_ns.namespace("noise")
NoiseComponent = noise_ns.class_("NoiseComponent", cg.Component)
NoiseWebServer = noise_ns.class_("NoiseWebServer", cg.Component)
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
NoiseSetSleepTimerAction = noise_ns.class_("NoiseSetSleepTimerAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseSetSpeakerVolumeAction = noise_ns.class_("NoiseSetSpeakerVolumeAction", automation.Action, cg.Parented.template(NoiseComponent))

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

CONF_SPEAKER_MEDIA_PLAYER = "speaker_media_player"
CONF_MASTER_MEDIA_PLAYER = "master_media_player"
CONF_AIRPLAY_RECEIVER = "airplay_receiver"
CONF_DUCK_ON_MEDIA_PLAYERS = "duck_on_media_players"
CONF_PAUSE_ON_MEDIA_PLAYERS = "pause_on_media_players"
CONF_DUCK_LEVEL = "duck_level"

CONF_SELECT = "select"
CONF_TONE = "tone"
CONF_SLEEP_TIMER = "sleep_timer"
CONF_TIME_LEFT = "time_left"
CONF_INITIAL_VOLUME = "initial_volume"
CONF_FADE_IN_TIME = "fade_in_time"
CONF_FADE_OUT_TIME = "fade_out_time"
CONF_ON_PLAY = "on_play"
CONF_ON_STOP = "on_stop"
CONF_ON_VARIANT_CHANGED = "on_variant_changed"
CONF_TITLE = "title"
CONF_SHOW_COMPONENTS = "show_components"

NOISE_WEB_SERVER_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NoiseWebServer),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
        cv.Optional(CONF_TITLE, default="Noise Machine"): cv.string,
        cv.Optional(CONF_SHOW_COMPONENTS, default=True): cv.boolean,
    }
)


def validate_noise_web_server(value):
    if value is True:
        return NOISE_WEB_SERVER_SCHEMA({})
    if value is False or value is None:
        return None
    if isinstance(value, dict):
        return NOISE_WEB_SERVER_SCHEMA(value)
    raise cv.Invalid("Expected boolean or web_server dictionary")

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
            cv.Optional(CONF_SPEAKER_MEDIA_PLAYER): cv.use_id(media_player.MediaPlayer),
            cv.Optional(CONF_MASTER_MEDIA_PLAYER): cv.use_id(media_player.MediaPlayer),
            cv.Optional(CONF_AIRPLAY_RECEIVER): cv.use_id(media_player.MediaPlayer),
            cv.Optional(CONF_DUCK_ON_MEDIA_PLAYERS): cv.ensure_list(cv.use_id(media_player.MediaPlayer)),
            cv.Optional(CONF_PAUSE_ON_MEDIA_PLAYERS): cv.ensure_list(cv.use_id(media_player.MediaPlayer)),
            cv.Optional(CONF_DUCK_LEVEL, default="20%"): cv.percentage,
            cv.Optional(CONF_SAMPLE_RATE, default=0): cv.int_range(min=0, max=48000),
            cv.Optional(CONF_CHANNELS, default=0): cv.int_range(min=0, max=2),
            cv.Optional(CONF_INITIAL_VOLUME, default="100%"): cv.percentage,
            cv.Optional(CONF_FADE_IN_TIME, default="100ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_FADE_OUT_TIME, default="100ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_WEB_SERVER): validate_noise_web_server,
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
            ).extend(
                {
                    cv.Optional(CONF_MIN_VALUE, default=0.0): cv.float_,
                    cv.Optional(CONF_MAX_VALUE, default=1440.0): cv.float_,
                    cv.Optional(CONF_STEP, default=1.0): cv.float_,
                }
            ),
            cv.Optional(CONF_TIME_LEFT): sensor.sensor_schema(
                unit_of_measurement="min",
                icon="mdi:timer-sand",
                device_class="duration",
                state_class="measurement",
                accuracy_decimals=0,
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

    speaker_mp_var = None
    if CONF_SPEAKER_MEDIA_PLAYER in config:
        speaker_mp_var = await cg.get_variable(config[CONF_SPEAKER_MEDIA_PLAYER])
    elif CONF_MASTER_MEDIA_PLAYER in config:
        speaker_mp_var = await cg.get_variable(config[CONF_MASTER_MEDIA_PLAYER])
    elif CONF_SPEAKER in config and "media_player" in core.CORE.config:
        speaker_id = str(config[CONF_SPEAKER])
        for mp_entry in core.CORE.config["media_player"]:
            if not isinstance(mp_entry, dict):
                continue
            spk = None
            if "announcement_pipeline" in mp_entry and isinstance(mp_entry["announcement_pipeline"], dict):
                spk = mp_entry["announcement_pipeline"].get("speaker")
            elif "media_pipeline" in mp_entry and isinstance(mp_entry["media_pipeline"], dict):
                spk = mp_entry["media_pipeline"].get("speaker")
            elif "speaker" in mp_entry:
                spk = mp_entry.get("speaker")
            if spk is not None and str(spk) == speaker_id:
                if CONF_ID in mp_entry:
                    speaker_mp_var = await cg.get_variable(mp_entry[CONF_ID])
                    break
    if speaker_mp_var is not None:
        cg.add_define("USE_MEDIA_PLAYER")
        cg.add(var.set_speaker_media_player(speaker_mp_var))

    if CONF_AIRPLAY_RECEIVER in config:
        cg.add_define("USE_NOISE_AIRPLAY")
        cg.add_define("USE_MEDIA_PLAYER")
        ap_var = await cg.get_variable(config[CONF_AIRPLAY_RECEIVER])
        cg.add(var.set_airplay_receiver(ap_var))

    if CONF_DUCK_ON_MEDIA_PLAYERS in config:
        cg.add_define("USE_MEDIA_PLAYER")
        for player_id in config[CONF_DUCK_ON_MEDIA_PLAYERS]:
            p_var = await cg.get_variable(player_id)
            cg.add(var.add_duck_source(p_var))

    if CONF_PAUSE_ON_MEDIA_PLAYERS in config:
        cg.add_define("USE_MEDIA_PLAYER")
        for player_id in config[CONF_PAUSE_ON_MEDIA_PLAYERS]:
            p_var = await cg.get_variable(player_id)
            cg.add(var.add_pause_source(p_var))

    if CONF_SELECT in config:
        cg.add_define("USE_SELECT")
        select_var = await select.new_select(
            config[CONF_SELECT],
            options=["Off"] + [v.capitalize() for v in VARIANTS],
        )
        cg.add(select_var.set_parent(var))
        cg.add(var.set_select(select_var))

    if CONF_MEDIA_PLAYER in config:
        cg.add_define("USE_MEDIA_PLAYER")
        mp_var = await media_player.new_media_player(config[CONF_MEDIA_PLAYER])
        cg.add(mp_var.set_parent(var))
        cg.add(var.set_media_player(mp_var))

    if CONF_VOLUME in config:
        cg.add_define("USE_NUMBER")
        vol_var = await number.new_number(
            config[CONF_VOLUME],
            min_value=0.0,
            max_value=100.0,
            step=1.0,
        )
        cg.add(vol_var.set_parent(var))
        cg.add(var.set_volume_number(vol_var))

    if CONF_TONE in config:
        cg.add_define("USE_NUMBER")
        tone_var = await number.new_number(
            config[CONF_TONE],
            min_value=0.0,
            max_value=100.0,
            step=1.0,
        )
        cg.add(tone_var.set_parent(var))
        cg.add(var.set_tone_number(tone_var))

    if CONF_SLEEP_TIMER in config:
        cg.add_define("USE_NUMBER")
        timer_conf = config[CONF_SLEEP_TIMER]
        timer_var = await number.new_number(
            timer_conf,
            min_value=timer_conf[CONF_MIN_VALUE],
            max_value=timer_conf[CONF_MAX_VALUE],
            step=timer_conf[CONF_STEP],
        )
        cg.add(timer_var.set_parent(var))
        cg.add(var.set_sleep_timer_number(timer_var))

    if CONF_TIME_LEFT in config:
        cg.add_define("USE_SENSOR")
        time_left_var = await sensor.new_sensor(config[CONF_TIME_LEFT])
        cg.add(var.set_time_left_sensor(time_left_var))

    if CONF_WEB_SERVER in config and config[CONF_WEB_SERVER] is not None:
        cg.add_define("USE_NOISE_WEB_SERVER")
        ws_conf = config[CONF_WEB_SERVER]
        base_var = await cg.get_variable(ws_conf[CONF_WEB_SERVER_BASE_ID])
        ws_var = cg.new_Pvariable(ws_conf[CONF_ID], var, base_var)
        cg.add(ws_var.set_title(ws_conf[CONF_TITLE]))
        cg.add(ws_var.set_show_components(ws_conf[CONF_SHOW_COMPONENTS]))
        await cg.register_component(ws_var, ws_conf)
        if core.CORE.using_arduino:
            cg.add_library("esphome/ESPAsyncWebServer-esphome", None)

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


@automation.register_action(
    "noise.set_sleep_timer",
    NoiseSetSleepTimerAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Required(CONF_SLEEP_TIMER): cv.templatable(cv.positive_float),
        }
    ),
    synchronous=True,
)
async def noise_set_sleep_timer_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_SLEEP_TIMER], args, cg.float_)
    cg.add(var.set_sleep_timer(template_))
    return var


@automation.register_action(
    "noise.set_speaker_volume",
    NoiseSetSpeakerVolumeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Required(CONF_VOLUME): cv.templatable(cv.percentage),
        }
    ),
    synchronous=True,
)
async def noise_set_speaker_volume_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_VOLUME], args, cg.float_)
    cg.add(var.set_volume(template_))
    return var