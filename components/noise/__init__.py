import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import speaker
from esphome.const import CONF_ID, CONF_SAMPLE_RATE, CONF_SPEAKER, CONF_VARIANT

DEPENDENCIES = ["speaker"]

noise_ns = cg.esphome_ns.namespace("noise")
NoiseComponent = noise_ns.class_("NoiseComponent", cg.Component)
NoiseStartAction = noise_ns.class_("NoiseStartAction", automation.Action, cg.Parented.template(NoiseComponent))
NoiseStopAction = noise_ns.class_("NoiseStopAction", automation.Action, cg.Parented.template(NoiseComponent))

VARIANTS = ["white", "pink", "brown", "gray"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NoiseComponent),
        cv.Required(CONF_SPEAKER): cv.use_id(speaker.Speaker),
        # 0 (default) = follow the speaker's configured sample rate.
        cv.Optional(CONF_SAMPLE_RATE, default=0): cv.int_range(min=0, max=48000),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    speaker_var = await cg.get_variable(config[CONF_SPEAKER])
    cg.add(var.set_speaker(speaker_var))


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