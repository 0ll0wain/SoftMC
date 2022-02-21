base_clk = 400 #Mhz
<<<<<<< HEAD
clkfbout_mult = 6
divclk_divide = 1
clkout_divide = 3
=======
clkfbout_mult = 8
divclk_divide = 1
clkout_divide = 4
>>>>>>> Software

DDR_clk = (base_clk * clkfbout_mult) / (divclk_divide * clkout_divide)
fabric_clk = DDR_clk / 2

print(DDR_clk)
<<<<<<< HEAD
print(fabric_clk)
=======
print(fabric_clk)
>>>>>>> Software
