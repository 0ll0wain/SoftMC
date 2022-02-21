base_clk = 400 #Mhz
clkfbout_mult = 6
divclk_divide = 1
clkout_divide = 3

DDR_clk = (base_clk * clkfbout_mult) / (divclk_divide * clkout_divide)
fabric_clk = DDR_clk / 2

print(DDR_clk)
print(fabric_clk)
