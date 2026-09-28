"""Validate the checked-out device YAML using synthetic credentials only."""

import argparse
import base64
import copy
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / ".ci-build"
SECRETS = {
    "wifi_ssid": "CI-NOT-A-REAL-NETWORK",
    "wifi_password": "ci-test-password",
    "fallback_hotspot_password": "ci-fallback-password",
    "encryption_key": base64.b64encode(bytes(range(32))).decode(),
}
VARIANTS = {
    "standalone": {"thermostat_entity": "false"},
    "linked": {"thermostat_entity": "climate.ci_test_room"},
    "overrides": {
        "thermostat_entity": "climate.ci_test_room",
        "humidity_icon": "mdi:water-percent",
        "humidity_icon_size": "32x32",
        "font_file": "gfonts://Open Sans",
        "menu_font_file": "gfonts://Roboto",
        "thermostat_font_size": "40",
    },
}


class TestLoader(yaml.SafeLoader):
    """Resolve !secret without ever opening the user's secrets.yaml."""


def secret(loader, node):
    name = loader.construct_scalar(node)
    if name not in SECRETS:
        raise ValueError(f"Add a synthetic CI value for secret: {name}")
    return SECRETS[name]


TestLoader.add_constructor("!secret", secret)


def prepare():
    config = yaml.load((ROOT / "esphome-round-thermostat.yaml").read_text(), Loader=TestLoader)
    # Resolve the declared assets from this checkout, never from a moving branch.
    packages = {}
    for name, source in config["packages"].items():
        for index, file in enumerate(source["files"]):
            path = (ROOT / file).resolve()
            if not path.is_relative_to(ROOT):
                raise ValueError(f"Package outside checkout: {file}")
            packages[f"{name}_{index}"] = yaml.safe_load(path.read_text())
    config["packages"] = packages
    for component in config["external_components"]:
        component["source"] = {"type": "local", "path": str(ROOT / "components")}
        component.pop("refresh", None)
    BUILD.mkdir(exist_ok=True)
    for mode, substitutions in VARIANTS.items():
        variant = copy.deepcopy(config)
        variant["substitutions"].update(substitutions)
        (BUILD / f"{mode}.yaml").write_text(yaml.safe_dump(variant, sort_keys=False, allow_unicode=True))
    print("Prepared standalone, linked and override configs from this checkout.")


def validate():
    from esphome.config import read_config
    from esphome.core import CORE

    configs = {}
    for mode in VARIANTS:
        CORE.reset()
        CORE.config_path = BUILD / f"{mode}.yaml"
        config = read_config({})
        if config is None:
            raise RuntimeError(f"Invalid configuration: {mode}")
        configs[mode] = config
        print(f"Valid configuration: {mode}")

    def assets(mode, kind):
        return {str(item["id"]): {k: v for k, v in item.items() if not k.endswith("id")}
                for item in configs[mode][kind]}

    for kind in ("image", "font"):
        assert assets("standalone", kind) == assets("linked", kind), kind
    defaults = assets("standalone", "image")
    icons = assets("overrides", "image")
    assert list(icons["room_humidity_icon"]["resize"]) == [32, 32]
    assert icons["room_humidity_icon"]["file"] != defaults["room_humidity_icon"]["file"]
    for name in defaults.keys() - {"room_humidity_icon"}:
        assert icons[name] == defaults[name], name
    fonts = assets("overrides", "font")
    default_fonts = assets("standalone", "font")
    assert fonts["thermostat_font"]["size"] == 40
    assert fonts["menu_font"] == default_fonts["menu_font"]
    for name in default_fonts.keys() - {"menu_font"}:
        assert fonts[name]["file"] != default_fonts[name]["file"], name
        assert fonts[name].get("glyphs") == default_fonts[name].get("glyphs"), name
    assert configs["standalone"]["round_thermostat"][0]["source_entity"] is False
    assert configs["linked"]["round_thermostat"][0]["source_entity"] == "climate.ci_test_room"
    print("Asset overrides, inherited defaults and climate source selection passed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("prepare", "validate"))
    args = parser.parse_args()
    prepare()
    if args.command == "validate":
        validate()
