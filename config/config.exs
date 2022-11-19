# This file is responsible for configuring your application
# and its dependencies with the aid of the Config module.
#
# This configuration file is loaded before any dependency and
# is restricted to this project.

# General application configuration
import Config

config :gems,
  namespace: GEMS

config :tailwind, version: "3.2.3", default: [
  args: ~w(
    --config=tailwind.config.js
    --input=css/app.css
    --output=../priv/static/assets/app.css
    --postcss
  ),
  cd: Path.expand("../assets", __DIR__)
]

# Configures the endpoint
config :gems, GEMSWeb.Endpoint,
  url: [host:  System.get_env("HOST") || System.get_env("PHX_HOST") || "localhost"],
  render_errors: [view: GEMSWeb.ErrorView, accepts: ~w(html json), layout: false],
  pubsub_server: GEMS.PubSub,
  live_view: [signing_salt: "SyZvC3lF"]

# Configure esbuild (the version is required)
config :esbuild,
  version: "0.12.18",
  default: [
    args:
      ~w(js/app.js --bundle --target=es2016 --outdir=../priv/static/assets --external:/fonts/* --external:/images/*),
    cd: Path.expand("../assets", __DIR__),
    env: %{"NODE_PATH" => Path.expand("../deps", __DIR__)}
  ]

# Configures Elixir's Logger
config :logger, :console,
  format: "$time $metadata[$level] $message\n",
  metadata: [:request_id]

# Use Jason for JSON parsing in Phoenix
config :phoenix, :json_library, Jason

# Import environment specific config. This must remain at the bottom
# of this file so it overrides the configuration defined above.
import_config "#{config_env()}.exs"
