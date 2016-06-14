target extended-remote : 3333

# Don't let GDB get confused while stepping
define hook-step
  mon cortex_m maskisr on
end
define hookpost-step
  mon cortex_m maskisr off
end

mon cortex_m maskisr auto
set print pretty
