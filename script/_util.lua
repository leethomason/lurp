function incMod(i, n)
    -- convert to 0 based
    i = i - 1
    n = n - 1
    return (i % n) + 1
end