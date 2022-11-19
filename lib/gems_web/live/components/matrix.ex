defmodule GEMSWeb.Components.Matrix do
  use GEMSWeb, :live_view
  use Phoenix.LiveComponent

  import Binary

  @spec active_column?(any, any) :: <<_::_*56>>
  def active_column?(current, col) when current == col do
    " active"
  end

  def active_column?(_current, _col), do: ""

  def cell_color(c) do
    "\##{Binary.to_hex(c)}"
  end
end
