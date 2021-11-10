pragma ton-solidity >=0.47.0;

library KWCPUtils {

    uint32 constant PODCAST_START = 1635600600;
    uint32 constant DAY_TIME = 86400;
    uint32 constant HOUR_TIME = 3600;
    uint32 constant MIN_TIME = 60;

    function timeBefore(uint32 time) internal pure returns(uint8 day, uint8 hour, uint8 min, uint8 sec) {
        //uint32 time = PODCAST_START - now;
        (uint32 d, uint32 r) = math.divmod(uint32(time), DAY_TIME);
        day = uint8(d);
        (d, r) = math.divmod(r, HOUR_TIME);
        hour = uint8(d);
        (d, r) = math.divmod(r, MIN_TIME);
        min = uint8(d);
        sec = uint8(r);
    }

    function timeAfter(uint32 time) internal pure returns(uint8 min, uint8 sec) {
        if (time > 3600) return (0,0);
        (uint32 d, uint32 r) = math.divmod(time, MIN_TIME);
        min = uint8(d);
        sec = uint8(r);
    }

    function unlockTime(uint128 timestamp) internal pure returns(string) {
        uint32 time = uint32(timestamp);
        if (time<=now) return "ready to unlock";
        string str = "unlocked in ";
        time = time - now;
        (uint32 d, uint32 r) = math.divmod(uint32(time), DAY_TIME);
        if (d == 1) {str.append(format("{} DAY ",d));} else {str.append(format("{} DAYS ",d));}
        (d, r) = math.divmod(r, HOUR_TIME);
        if (d == 1) {str.append(format("{} HOUR ",d));} else {str.append(format("{} HOURS ",d));}
        (d, r) = math.divmod(r, MIN_TIME);
        if (d == 1) {str.append(format("{} MINUTE ",d));} else {str.append(format("{} MINUTES ",d));}
        if (r == 1) {str.append(format("{} SECOND",r));} else {str.append(format("{} SECONDS",r));}
        return str;
    }

    function getKWTPrice (uint8 minute) internal pure returns(uint128) {
        return 0.1 ton + 0.01 ton * uint128(minute);
    }

    function getKWTStrPrice (uint8 minute) internal pure returns(string) {
        uint128 p = getKWTPrice(minute);
        (uint128 dec, uint128 float) = math.divmod(p, 1 ton);
        float = uint128(float / 10000000);
        string result = format("{}.{:02}", dec, float);
        return result;
    }

    function nanoton2str (uint128 nt) internal pure returns(string) {
        return format("{}", uint128(nt / 1 ton));
    }

    function getTotalSupplyAtMinute (uint8 minute) internal pure returns(string) {
        uint32[] array = [50000,55713,62146,69399,77583,86828,97286,109126,122549,137782,155090,174780,197204,222774,251967,285337,323528,367291,417504,475190,541547,617977,706126,807925,925646,1061963,1220030,1403570,1616987,1865493,2155269,2493656,2889382,3352841,3896432,4534964,5286151,6171219,7215637,8450014,9911195,11643600,13700870,16147884,19063243,22542327,26701075,31680646,37653207,44829097,53465742,63878742,76455708,91673543,110120064,132521127,159774686,192993665,233560009,283192970];
        return format("{}", array[minute]);
    }

    function kwt2str(uint128 token) internal pure returns(string) {
        (uint128 d, uint128 r) = math.divmod(token, uint128(1000));
        string str = format("{}",d);
        if(r>0) str.append(format(".{:03}",r));
        return str;
    }

}
