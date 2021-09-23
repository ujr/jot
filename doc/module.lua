-- Sample Module Structure

local M = {}                   -- this is our module

local sqrt = math.sqrt         -- import section: declare what
local io = io                  -- this module needs from the outside

_ENV = nil                     -- no more external access after this point!

local function foo() return sqrt(2) end -- private function

function M.bar() return 2*foo() end     -- exported function
M.baz = function() return foo()/3 end   -- ditto

return M                       -- the result of `require "module"`
