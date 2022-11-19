defmodule GEMSWeb.Websockets.GEMSSocket do
  @moduledoc """
  `Phoenix.Socket.Transport` implementation for sending matrix images
  to the client connection.
  """

  @behaviour Phoenix.Socket.Transport

  alias GEMS.MatrixStore, as: Store

  import Bitwise

  require Logger

  @impl true
  def child_spec(_opts) do
    # Nothing to do here, so noop.
    %{id: Task, start: {Task, :start_link, [fn -> :ok end]}, restart: :transient}
  end

  @impl true
  def connect(%{params: %{"token" => token, "rate" => rate}}) do
    debug_msg(fn -> "connection with : #{token}  / #{rate}" end)

    {:ok, %{token: token, rate: String.to_integer(rate)}}
  end

  @impl true
  def init(%{token: token, rate: rate} = args) do
    debug_msg(fn -> "init with #{inspect(args)}" end)
    send(self(), :send_image)
    {:ok, %{next_image_in: rate}}
  end

  @impl true
  def handle_in({message, opts}, state) do
    debug_msg(fn -> "handle_in with message: #{inspect(message)}, opts: #{inspect(opts)}" end)
    {:ok, state}
  end

  @impl true
  def handle_info(:send_image, %{next_image_in: send_after} = state) do
    debug_msg(fn -> "handling send_image, next in #{send_after}" end)
    Process.send_after(self(), :send_image, send_after)

    # TODO: configurable color depth
    data = Store.get()
    if data do
      {:push, {:binary, data }, state}
    end
  end

  def handle_info({:rate_change, new_next_image_in}, state) do
    {:ok, %{state | next_image_in: new_next_image_in}}
  end

  def handle_info(_, state) do
    {:ok, state}
  end

  @impl true
  def terminate(reason, _state) do
    debug_msg(fn -> "terminating because #{inspect(reason)}" end)
    :ok
  end

  defp debug_msg(msg_fn) do
    Logger.debug(fn -> "GEMSocket #{inspect(self())} " <> msg_fn.() end)
  end
end
RED
