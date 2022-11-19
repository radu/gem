defmodule GEMSWeb.Components.ColorPicker do
  use GEMSWeb, :live_view
  use Phoenix.LiveComponent

  def color_swatches() do
   [ 0xF8F8F8,
     0xB0B4B8,
     0x00B858,
     0x98F068,
     0x28F8A8,
     0x0000FF,
     0xFF0000,
     0xFFFF40,
     0x000000,
     0x686C78,
     0x10A010,
     0x105488,
     0x0098F8,
     0x4038B8,
     0xA83050,
     0xF87448
    ]
  end

  def color_string(c) do
    "\##{Integer.to_string(c, 16) |> String.pad_leading(6, [ "0" ] )}"
  end

  def color_num(c) do
    (c.r * 256 + c.g)*256+c.b
  end

  def graph(
    %{
      w: w,
      h: h,
      color: c
    } = assigns
  ) do
  IO.inspect(c, label: "Graph color")
  fill = c |> IO.inspect(label: 'c') |> color_num |> IO.inspect(label: 'color_num') |> color_string |> IO.inspect(label: 'fill')
~H"""
<svg height={h} width={w} viewBox={"0 0 #{w} #{h}"} xmlns="http://www.w3.org/2000/svg">
  <circle cx={h/3} cy={w/3} r={min(h,w) * 0.25} fill={fill}/>
</svg>
"""
end

end
