defmodule GEMS.Matrix.ColorMatrix do
  @moduledoc """
  Abstracts a 3-byte RGB + 1-byte ID matrix into a byte array

  Example:

  ```
  # 16 points in the following matrix:
  [ 1, 0, 0, 0]
  [ 0, 1, 0, 0]
  [ 0, 0, 1, 0]
  [ 0, 0, 0, 1]
  ```
  The 16 points in the matrix above can be represeted by a 1-dimensional array/list/bitstring
  of `64 bytes` ( 16 x 4 bytes).
"""
  @behaviour GEMS.Matrix

  defstruct board: "", h: 32, w: 64


  @doc """
  Create a square matrix
"""
  @impl GEMS.Matrix
  def new(size, opts \\ []) do
    grid_size = 64 * size
    data_size = grid_size * 3

    board = Keyword.get(opts, :board) || zero_grid(grid_size)

    if byte_size(board) == data_size do
      {:ok, %__MODULE__{board: board, h: size, w: 64}}
    else
      {:error, :invalsizeid_board}
    end
  end

  @doc """
  Get an element from the matrix
  """
  @impl GEMS.Matrix
  def get(%{board: b, w: w}, x, y) do
    binary_part(b, calc_index(x, y ,w), 3)
  end

  @spec set(%{:board => binary, :w => number, optional(any) => any}, number, number, binary) :: %{
          :board => binary,
          :w => number,
          optional(any) => any
        }
  @doc """
  Set an element in the matrixboardboardboard
  """
  @impl GEMS.Matrix
  def set(%{board: b, w: w} = grid, x, y, c888 = <<_c::24>>) do
    mat_size = byte_size(b) |> IO.inspect(label: 'matrix size')

    pix_addr = calc_index(x, y, w) |> IO.inspect(label: 'pixel address')
    pix_after = mat_size - pix_addr - 3 |> IO.inspect(label: 'pixels after')

    prev_bytes = binary_part(b, 0, pix_addr)
    next_bytes = binary_part(b, pix_addr + 3, pix_after)

    # c16 = c24 |> CvtColor.cvt(:rbg888, :rgb565)

    %{grid | board: prev_bytes <> c888 <> next_bytes}
  end

  defp calc_index(x, y, w) do
    ( y * w + x ) * 3
  end

  defp zero_grid(grid_size) do
    Binary.copy(<<0::24>>, grid_size)
  end
end
