function abortIf(var, message)
    if var then
       error(message, 0)
    end
end

function instanceOf(instance, desiredType)
    if type(instance) ~= 'table' then
        return false
    end
    mt = getmetatable(instance)
    while mt ~= nil do
        if mt == desiredType then
            return true
        end
        mt = getmetatable(mt)
    end
    return false
end

-- Simple target
Target = { }
_meiqueAllTargets = {}
_meiqueCurrentDir = {'.'}

function currentDir()
    if #_meiqueCurrentDir > 1 then
        return table.concat(_meiqueCurrentDir, '/')..'/'
    else
        return './'
    end
end

-- Object used for disabled scopes
_meiqueNone = {}
-- Object used for enabled scopes
_meiqueNotNone = {}

_meiqueNone.__call = function(func, ...)
    return _meiqueNone
end
_meiqueNone.__index = _meiqueNone.__call
_meiqueNone.__eval = function(table)
    return false
end
_meiqueNone.__not = function(table)
    return _meiqueNotNone
end

_meiqueNotNone.__index = function(table, key)
    local typeName = type(_G[key])
    if typeName == 'function' then
        return function(a, ...) return _G[key](...) end
    elseif typeName == 'nil' then
        return nil
    else
        return function() return _G[key] end
    end
end
_meiqueNone.__eval = function(table)
    return true
end
_meiqueNotNone.__not = function(table)
    return _meiqueNone
end

setmetatable(_meiqueNone, _meiqueNone)
setmetatable(_meiqueNotNone, _meiqueNotNone)

-- Scopes: build types
DEBUG = _meiqueNone
RELEASE = _meiqueNone

-- Scopes: Platforms
LINUX = _meiqueNone
UNIX = _meiqueNone
WINDOWS = _meiqueNone
MACOSX = _meiqueNone

-- Scopes: Compilers
GCC = _meiqueNone
MSVC = _meiqueNone
MINGW = _meiqueNone


-- Constants
OPTIONAL = true

function addSubDirectory(dir)
    local strDir = tostring(dir)
    table.insert(_meiqueCurrentDir, dir)
    local fileName = currentDir()..'meique.lua'
    local func, error = loadfile(fileName, fileName)
    abortIf(func == nil, error)
    func()
    table.remove(_meiqueCurrentDir)
end

function Target:new(name)
    abortIf(name == nil and name ~= Target, 'Target name can\'t be nil')
    o = {}
    setmetatable(o, Target)
    Target.__index = Target
    if type(name) ~= 'table' then
        o._name = name
        o._files = {}
        o._deps = {}
        o._dir = currentDir()
        o._tests = {}
        o._preTargetCompileHooks = {}
        o._installFiles = {}
        _meiqueAllTargets[tostring(name)] = o
    end
    return o
end

function Target:sourceDir()
    return meiqueSourceDir()..self._dir
end

function Target:buildDir()
    return meiqueBuildDir()..self._dir
end

function Target:addFiles(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._files, f) end)
    end
end

function Target:addDependency(target)
    abortIf(not instanceOf(target, Target), 'Expected a target, got something different.')
    table.insert(self._deps, target._name)
end

function Target:addTest(testCommand, testName)
    if instanceOf(testCommand, Executable) then
        table.insert(self._tests, {testCommand._name, testCommand:buildDir()..testCommand._name, currentDir()})
    else
        testName = testName or self._name..'-test #'..#self._tests
        table.insert(self._tests, {testName, testCommand, currentDir()})
    end
end

function Target:install(srcs, destDir)
    if destDir then
        installEntry = {destDir}
        string.gsub(srcs, '([^%s]+)', function(f) table.insert(installEntry, f) end)
        table.insert(self._installFiles, installEntry )
    else
        table.insert(self._installFiles, {srcs})
    end
end

-- Custom target
CustomTarget = Target:new(Target)

function CustomTarget:new(name, func)
    o = Target:new(name)
    setmetatable(o, self)
    self.__index = self
    o._func = func
    o._type = 3
    return o
end

-- Compilable target
CompilableTarget = Target:new(Target)

function CompilableTarget:new(name)
    o = Target:new(name)
    setmetatable(o, self)
    self.__index = self
    o._incDirs = {}
    o._libDirs = {}
    o._linkLibraries = {}
    o._packages = {}
    o._targets = {}
    return o
end

function CompilableTarget:addIncludeDirs(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._incDirs, f) end)
    end
end

function CompilableTarget:addLibIncludeDirs(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._libDirs, f) end)
    end
end

function CompilableTarget:addLinkLibraries(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._linkLibraries, f) end)
    end
end

function CompilableTarget:usePackage(package)
    abortIf(type(package) ~= 'table', 'Package table expected, got '..type(package))
    table.insert(self._packages, package)
end

function CompilableTarget:useTarget(target)
    abortIf(not instanceOf(target, Library), 'You can only use library targets on other targets.')
    table.insert(self._targets, target._name)
    self:addDependency(target)
end

-- Executable
Executable = CompilableTarget:new(Target)
function Executable:new(name)
    o = CompilableTarget:new(name)
    setmetatable(o, self)
    self.__index = self
    o._type = 1
    return o
end

SHARED = 1
STATIC = 2
-- Library
Library = CompilableTarget:new(Target)
function Library:new(name, libType)
    o = CompilableTarget:new(name)
    setmetatable(o, self)
    self.__index = self
    o._libType = libType or SHARED
    o._type = 2
    return o
end


-- Qt extensions

function CompilableTarget:useQtAutomoc()
    table.insert(self._preTargetCompileHooks, _meiqueAutomoc)
end
