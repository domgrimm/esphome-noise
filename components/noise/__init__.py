import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import select, speaker
from esphome.const import CONF_ID, CONF_SAMPLE_RATE, CONF_SPEAKER, CONF_VARIANT

DEPENDENCIES = ["speaker"]
AUTO_LOAD = ["select"]

noise_ns = cg.esphome_ns.namespace("noise")
NoiseComponent = noise_ns.class_("NoiseComponent", cg.Component)
NoiseSelect = noise_ns.class_("NoiseSelect", select.Select)
NoiseStartAction = noise_ns.class_("NoiseStartAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseStopAction = noise_ns.class_("NoiseStopAction", automation.Action, cg.Parented.template(NoiseComponent))

VARIANTS = ["white", "pink", "brown", "gray", "waves", "wind", "stream", "fan"]

CONF_SELECT = "select"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NoiseComponent),
        cv.Required(CONF_SPEAKER): cv.use_id(speaker.Speaker),
        # 0 (default) = follow the speaker's configured sample rate.
        cv.Optional(CONF_SAMPLE_RATE, default=0): cv.int_range(min=0, max=48000),
        # Auto-created "Noise" select: options derive from the variant list, and
        # changing it starts/stops playback. Add a `name:` to expose it in HA.
        cv.Optional(CONF_SELECT): select.select_schema(
            NoiseSelect,
            entity_category=cv.UNDEFINED,
            icon="mdi:white-balance-sunny",
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    speaker_var = await cg.get_variable(config[CONF_SPEAKER])
    cg.add(var.set_speaker(speaker_var))

    if CONF_SELECT in config:
        select_var = await select.new_select(
            config[CONF_SELECT],
            options=["Off"] + [v.capitalize() for v in VARIANTS],
        )
        cg.add(select_var.set_parent(var))
        cg.add(select_var.publish_state("Off"))


@automation.register_action(
    "noise.start",
    NoiseStartAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(NoiseComponent),
            cv.Required(CONF_VARIANT): cv.templatable(cv.one_of(*VARIANTS, lower=True)),
        }
    ),
    synchronous=True,
)
async def noise_start_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_VARIANT], args, cg.std_string)
    cg.add(var.set_variant(template_))
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