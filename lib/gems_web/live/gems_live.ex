defmodule GEMSWeb.GEMSLive do
  use GEMSWeb, :live_view

  import GEMS.Util.Time

  import RandomColor

  alias GEMS.Matrix
  alias GEMSWeb.PubSub
  alias GEMSWeb.Presence
  alias GEMS.MatrixStore, as: Store

  @default_size 32
  @default_tempo 180
  @local_default %{
    color: List.zip( [[:r, :g, :b],
                      RandomColor.rgb(format: :tuple) |> Tuple.to_list ])
                      |> Map.new(),
    key: 0,
    scale: 0,
    tempo: @default_tempo,
    reverb: 10,
    delay: 10,
    adsr: %{
      a: 1,
      d: 25,
      s: 15,
      r: 35
    }
  }

  def mount(params, _, socket) do
    if connected?(socket), do: Process.send_after(self(), :clear_flash, 5000)

    topic = room_topic(params)

    if public_room?(topic) do
      board = Store.get()
      new_matrix(@default_size, board)
    else
      new_matrix_64(matrix_size(params), Map.get(params, "m"))
    end
    |> case do
      {:ok, matrix} ->
        PubSub.subscribe(topic)
        Presence.track(self(), topic, socket.id, %{})

        {:ok,
         assign(socket,
           topic: topic,
           local: @local_default,
           global: %{
             matrix: matrix,
             users: 1
           }
         )}

      _error ->
        # Simply redirect to homepage if any errors
        socket = put_flash(socket, :error, "Invalid url. Redirected.")

        mount(%{}, nil, push_redirect(socket, to: "/"))
    end
  end

  def handle_params(_, _, socket) do
    {:noreply, socket}
  end

  def handle_event(
    "color-pick",
    %{"hex" => c},
    %{assigns: %{topic: _topic, local: local}} = socket
  ) do
    <<r::8, g::8, b::8>> = c |> String.to_integer |> Binary.from_integer |> Binary.pad_leading(3)
    color = %{r: r, g: g, b: b}
    {:noreply, assign(socket, local: %{local | color: color })}
  end

  def handle_event(
    "color",
    %{"_target" => [_, key], "slider" => value},
    %{assigns: %{topic: _topic, local: local}} = socket
    ) do
    IO.inspect(value, label: 'color select')

    {v, ""} =
      Map.get(value, key)
      |> Integer.parse()

    # PubSub.broadcast(topic, {:color_update, %{c: c}})

    {:noreply, assign(socket, local: %{local | color: Map.put(local[:color], String.to_atom(key), v)})}
  end

  def handle_event(
        "matrix-item",
        %{"col" => x, "row" => y},
        %{assigns: %{topic: topic, local: local}} = socket
      ) do
    {x, ""} = Integer.parse(x)
    {y, ""} = Integer.parse(y)

    IO.inspect(local.color, label: 'current color')

    PubSub.broadcast(topic, {:matrix_update, %{x: x, y: y, c: local.color}})

    {:noreply, socket}
  end

  # handle updates from ADSR sliders
  def handle_event(
        "adsr",
        %{"_target" => [_, key], "slider" => value},
        %{assigns: %{local: local}} = socket
      ) do
    {v, ""} =
      Map.get(value, key)
      |> Integer.parse()

    {:noreply,
     assign(socket, local: %{local | adsr: Map.put(local[:adsr], String.to_atom(key), v)})}
  end

  # general-use updates from local-control sliders
  def handle_event(
        "local-control",
        %{"_target" => [_, key], "slider" => value},
        %{assigns: %{local: local}} = socket
      ) do
    {v, ""} =
      Map.get(value, key)
      |> Integer.parse()

    {:noreply, assign(socket, :local, Map.put(local, String.to_atom(key), v))}
  end

  # general-use updates from local-control < and > buttons (+/-)
  def handle_event(
        "inc",
        %{"action" => action, "value" => key, "max" => max},
        %{assigns: %{local: local}} = socket
      ) do
    {max, ""} = Integer.parse(max)
    key = String.to_existing_atom(key)
    current = Map.get(local, key)

    new =
      case action do
        "+" -> current + 1
        "-" -> current - 1
      end

    # wrap around
    new =
      cond do
        new >= max ->
          -1 * max

        new <= -1 * max ->
          max

        true ->
          new
      end

    # rem(new + max, max)

    {:noreply, assign(socket, :local, Map.put(local, key, new))}
  end

  def handle_event("reset", _, %{assigns: %{topic: topic}} = socket) do
    PubSub.broadcast(topic, :matrix_reset)

    {:noreply, socket}
  end

  def handle_info(
        :matrix_reset,
        %{assigns: %{topic: topic, global: %{matrix: m} = g}} = socket
      ) do
    with {:ok, m} <- new_matrix(m.h) do
      socket =
        assign(socket, :global, %{g | matrix: m})
        |> save_board_to_url()

      if public_room?(topic) do
        Store.update(m.board, now())
      end

      {:noreply, assign(socket, :global, %{g | matrix: m})}
    end
  end

  def handle_info(
        {:matrix_update, %{c: %{r: r, g: g, b: b}, x: x, y: y}},
        %{assigns: %{topic: topic, global: %{matrix: m} = glob}} = socket
      ) do
    socket = save_board_to_url(socket)

    c = <<r::8>> <> <<g::8>> <> <<b::8>>

    m = Matrix.set(m, x, y, c)

    if public_room?(topic) do
      Store.update(m.board, now())
    end

    {:noreply, assign(socket, :global, %{glob | matrix: m})}
  end

  # callback for Presence when a user connects/disconnects
  def handle_info(
        %{event: "presence_diff", payload: _payload},
        %{assigns: %{topic: topic, global: g}} = socket
      ) do
    {:noreply, assign(socket, :global, %{g | users: Presence.user_count(topic)})}
  end

  def handle_info(:clear_flash, socket) do
    {:noreply, clear_flash(socket)}
  end

  defp save_board_to_url(%{assigns: %{topic: topic, global: %{matrix: m}}} = socket) do
    if(public_room?(socket)) do
      socket
    else
      room = room_name(topic)
      url = Routes.room_gems_path(socket, :show, room, s: m.w, m: Base.url_encode64(m.board))
      push_patch(socket, to: url)
    end
  end

  defp matrix_size(%{"s" => s}) when s in ["32", "64"] do
    String.to_integer(s)
  end

  defp matrix_size(_), do: @default_size
end
