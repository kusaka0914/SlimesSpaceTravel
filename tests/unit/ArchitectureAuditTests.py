import unittest

from tools.architecture_audit import dependency_violations, game_accesses, header_cycles, inheritance_declarations


class ArchitectureAuditTests(unittest.TestCase):
    def test_dependency_gate_rejects_forbidden_directions(self):
        edges = [
            {"from": "src/gfx/Renderer.cpp", "to": "src/editor/Editor.h"},
            {"from": "src/system/Loader.cpp", "to": "src/editor/Panel.h"},
            {"from": "src/utils/File.cpp", "to": "src/app/Game.h"},
            {"from": "src/system/camera/PlayerCamera.cpp", "to": "src/app/Game.h"},
        ]
        self.assertEqual(len(dependency_violations(edges)), 4)

    def test_dependency_gate_allows_composition_and_domain_dependencies(self):
        edges = [
            {"from": "src/app/Game.cpp", "to": "src/editor/Editor.h"},
            {"from": "src/editor/Panel.cpp", "to": "src/world/Stage.h"},
            {"from": "src/actor/Player.cpp", "to": "src/app/Game.h"},
            {"from": "src/utils/File.cpp", "to": "src/utils/AtomicFile.h"},
        ]
        self.assertEqual(dependency_violations(edges), [])

    def test_header_cycle_gate_ignores_cpp_edges_and_detects_real_cycle(self):
        edges = [{"from": "a.h", "to": "b.h"}, {"from": "b.h", "to": "a.h"},
                 {"from": "use.cpp", "to": "a.h"}, {"from": "a.h", "to": "use.cpp"}]
        self.assertEqual(header_cycles(edges), [("a.h", "b.h")])

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
