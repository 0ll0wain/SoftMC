base_clk = 200 #Mhz
clkfbout_mult = 10
divclk_divide = 1
clkout_divide = 4

DDR_clk = (base_clk * clkfbout_mult) / (divclk_divide * clkout_divide)
fabric_clk = DDR_clk / 2

print(DDR_clk)
print(fabric_clk)
