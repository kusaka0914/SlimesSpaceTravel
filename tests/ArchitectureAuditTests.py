import unittest

from tools.architecture_audit import game_accesses, inheritance_declarations


class ArchitectureAuditTests(unittest.TestCase):
    def test_game_accesses_include_references_typed_aliases_and_accessor_chains(self):
        source = """Game& application = owner;
mGame->GetWindow();
game.GetInputSystem();
mContext.game->LoadStage();
player.GetGame()->GetPhysicsSystem();
application.GetPlayers();
"""
        calls = list(game_accesses(source))
        self.assertEqual(
            [call["api"] for call in calls],
            ["GetWindow", "GetInputSystem", "LoadStage", "GetPhysicsSystem", "GetPlayers"],
        )
        self.assertEqual([call["line"] for call in calls], [2, 3, 4, 5, 6])

    def test_game_accesses_ignore_comments_strings_and_unrelated_receivers(self):
        source = '''// game->Ignored();
/* mGame.GetWindow(); */
const char* text = "game.GetInputSystem()";
otherGame->Ignored();
Game* application = nullptr;
application->RunLoop();
'''
        self.assertEqual(
            list(game_accesses(source)),
            [{"line": 6, "receiver": "application", "api": "RunLoop"}],
        )

    def test_inheritance_includes_final_structs_and_multiple_bases(self):
        source = """class Actor : public Base {};
class Camera final : public Callback {};
struct Adapter : First, public Second {};
"""
        self.assertEqual(
            list(inheritance_declarations(source)),
            [
                {"line": 1, "class": "Actor", "bases": "public Base"},
                {"line": 2, "class": "Camera", "bases": "public Callback"},
                {"line": 3, "class": "Adapter", "bases": "First, public Second"},
            ],
        )

    def test_qualified_implementation_is_not_inheritance(self):
        source = '''class Video::Implementation {};
// class Fake : No {};
const char* text = "class NotCode : Base {}";
class Video::Derived final : public Base {};
'''
        self.assertEqual(
            list(inheritance_declarations(source)),
            [{"line": 4, "class": "Video::Derived", "bases": "public Base"}],
        )
