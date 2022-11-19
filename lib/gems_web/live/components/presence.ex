defmodule GEMSWeb.Components.Presence do
  use Phoenix.Component

  def count(assigns) do
    assigns = assign(assigns, :users, assigns.users )
    ~H"""
    <div class="presence">
      <span><%= @users %></span>
      <img class="human"/>
    </div>
    """
  end
end
