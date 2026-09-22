import lit.formats
import os

config.name = "Decoder"
config.test_format = lit.formats.ShTest(False)
config.suffixes = [".test"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.my_obj_root, "tests", "lit") \
    if hasattr(config, "my_obj_root") else config.test_source_root

decoder_bin = os.environ.get("DECODER_BIN", "../../build/decoder")
config.substitutions.append(("%decoder", decoder_bin))
