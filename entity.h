enum EntityFlag {
    ENTITY_FLAG_MOVE_PLAYER,
};

struct DefaultEntityAnimations {
    Animation idle;
	Animation run;

    Animation hurt;
    Animation death;
	
	Animation attackUp;
    Animation attackDown;
    Animation attackSide;

    Animation work;

    Animation scared;
};


struct Entity {
    u64 id;
    u64 flags;

    TransformX T;
    float speed;
    float3 velocity;

    EasyAnimation_Controller animationController;
    DefaultEntityAnimations *defaultAnimations;

    float deltaTLeft;
    float3 deltaPos;
};