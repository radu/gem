defmodule GEMSWeb.PageController do
  require Logger
  use GEMSWeb, :controller

  def index(conn, _params) do
    Logger.debug("PageController #{inspect(self())} " <> conn )
    render(conn, "index.html")
  end
end
