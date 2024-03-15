from SCons.Script import DefaultEnvironment, Return
from getpass import getpass
import json
import os.path

env = DefaultEnvironment()

# Do not run a script when external applications, such as IDEs,
# dump integration data. Otherwise, input() will block the process
# waiting for the user input
if env.IsIntegrationDump():
    # stop the current script execution
    Return()

CREDENTIALS_PATH = "credentials.json"

# read from file
if os.path.exists(CREDENTIALS_PATH):
    print(f"{CREDENTIALS_PATH} found")

    with open(CREDENTIALS_PATH) as f:
        credentials = json.loads("\n".join(f.readlines()))
        ssid: str = credentials["ssid"]
        passwd: str = credentials["passwd"]
else:
    # read from stdin
    print(f"{CREDENTIALS_PATH} cannot be found")
    print("Provide your network credentials")
    ssid = input("SSID:\n")
    passwd = getpass("Password:\n")

env.Append(
    CPPDEFINES=[
        ("EXAMPLE_WIFI_SSID",  env.StringifyMacro(ssid)),
        ("EXAMPLE_WIFI_PASSWORD",  env.StringifyMacro(passwd))
    ],
)
